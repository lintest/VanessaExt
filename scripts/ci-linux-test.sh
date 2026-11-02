#!/usr/bin/env bash
#
# Прогон автотестов VanessaExt в тонком клиенте 1С под Linux.
#
# Зачем отдельный контур: сборка .so проверяет только компиляцию и ABI, но не
# то, что компонента реально загружается и работает. Здесь мы поднимаем Xvfb
# с оконным менеджером, ставим учебную поставку платформы 1С, собираем .epf с
# Linux-библиотекой внутри и гоняем тот же набор тестов, что и на Windows.
#
# Почему ubuntu-22.04, а не 18.04: платформа собрана под WebKitGTK 4.0
# (libwebkit2gtk-4.0.so.37, libsoup-2.4), в 24.04 их уже нет. Заодно это
# проверка ABI-базовой линии: .so собрана на 18.04 (glibc 2.27) и должна
# загрузиться на более новой системе.
#
# Ожидает переменные:
#   KEY_1CV8T      — пароль к архиву платформы в lintest/tools1c (ветка platform-linux);
#                    нужен только если архив зашифрован
#   ADDIN_ZIP      — путь к AddIn.zip с Linux-библиотеками (артефакт job'а linux)
# Необязательные:
#   PLATFORM_BRANCH (по умолчанию platform-linux)
#   WORKDIR         (по умолчанию текущий каталог)
set -euo pipefail

WORKDIR="${WORKDIR:-$(pwd)}"
PLATFORM_BRANCH="${PLATFORM_BRANCH:-platform-linux}"
DIST="${HOME}/1c/dist"
DB="${WORKDIR}/database"
PROJECT="VanessaExt"

DISPLAY_NUM="${DISPLAY_NUM:-99}"
export DISPLAY=":${DISPLAY_NUM}"

log() { echo "== $*"; }

fail() {
	echo "::error::$*" >&2
	exit 1
}

# Печать многострочного текста одной аннотацией GitHub Actions. Логи шага и
# артефакты видны только владельцу репозитория, а аннотации check-run отдаются
# публичным API — поэтому ключевую диагностику дублируем сюда. Переводы строк
# и проценты экранируются по правилам workflow-команд.
annotate() {
	local level="$1"
	local text="$2"
	text="${text//'%'/%25}"
	text="${text//$'\r'/}"
	text="${text//$'\n'/%0A}"
	echo "::${level}::${text:0:6000}"
}

cd "${WORKDIR}"

# --- 1. Зависимости -------------------------------------------------------
log "Зависимости"
# Вывод apt подавлен: он занимает десятки тысяч строк и вытесняет из лога
# шага то, ради чего лог читают, — хвост с логами платформы 1С при падении.
sudo apt-get update -qq >/dev/null
sudo apt-get install -y -qq --no-install-recommends \
	p7zip-full xvfb x11-utils openbox \
	libwebkit2gtk-4.0-37 libjavascriptcoregtk-4.0-18 libsoup2.4-1 \
	libglu1-mesa libgtk-3-0 libxtst6 libxrender1 libxi6 libglib2.0-0 \
	libnss3 libxext6 libsm6 libice6 libfreetype6 libfontconfig1 \
	libdbus-1-3 libasound2 libxslt1.1 libxfixes3 libxcursor1 \
	libxrandr2 libcups2 libpango-1.0-0 libcairo2 libgbm1 libpng16-16 \
	locales fonts-dejavu-core zip unzip dbus dbus-x11 >/dev/null

# Кириллица в сообщениях тестов: без локали 1С падает на выводе.
sudo locale-gen ru_RU.UTF-8 >/dev/null 2>&1 || true

# --- 2. Платформа 1С ------------------------------------------------------
# Архив лежит томами linux1c.7z.001, .002, ... в ветке platform-linux.
# Каталог DIST кэшируется в workflow, поэтому скачивание выполняется один раз
# на версию ветки.
if [ ! -f "${DIST}/linux1c.7z.001" ]; then
	log "Скачивание платформы (${PLATFORM_BRANCH})"
	mkdir -p "${DIST}"
	base="https://raw.githubusercontent.com/lintest/tools1c/${PLATFORM_BRANCH}"
	for i in $(seq -w 1 40); do
		if ! curl -fsSL -o "${DIST}/linux1c.7z.0${i}" "${base}/linux1c.7z.0${i}"; then
			rm -f "${DIST}/linux1c.7z.0${i}"
			break
		fi
	done
else
	log "Платформа уже скачана (кэш)"
fi

ls -la "${DIST}" | head -n 5

if [ -z "$(ls /opt/1cv8t/x86_64/*/1cv8ct 2>/dev/null || true)" ]; then
	log "Распаковка и установка платформы"
	# Архив бывает как без пароля, так и под ним — сначала пробуем без ключа,
	# иначе 7z падает с «Cannot open encrypted archive» на открытом архиве.
	if ! 7z x -y -o"${DIST}/unpacked" "${DIST}/linux1c.7z.001" > /dev/null 2>&1; then
		log "архив под паролем, пробую KEY_1CV8T"
		[ -n "${KEY_1CV8T:-}" ] || fail "архив зашифрован, а KEY_1CV8T не задан"
		7z x -y -p"${KEY_1CV8T}" -o"${DIST}/unpacked" "${DIST}/linux1c.7z.001" > /dev/null \
			|| fail "не удалось распаковать архив платформы (проверьте KEY_1CV8T)"
	fi

	run=$(find "${DIST}/unpacked" -maxdepth 4 -name '*.run' | head -n 1)
	[ -n "${run}" ] || fail "установщик .run не найден в архиве"
	chmod +x "${run}"
	sudo "${run}" --mode unattended --unattendedmodeui none \
		--enable-components client_full,ws,ru
	rm -rf "${DIST}/unpacked"
else
	log "Платформа уже установлена"
fi

bin=$(ls /opt/1cv8t/x86_64/*/1cv8t 2>/dev/null | head -n 1)
client=$(ls /opt/1cv8t/x86_64/*/1cv8ct 2>/dev/null | head -n 1)
[ -n "${bin}" ] || fail "1cv8t не найден после установки"
[ -n "${client}" ] || fail "1cv8ct не найден после установки"
log "Конфигуратор: ${bin}"
log "Тонкий клиент: ${client}"

# --- 3. Дисплей и оконный менеджер ---------------------------------------
# Оконный менеджер нужен не для красоты: Тест_СвойстваУправлениеОкнами и
# Тест_МетодыУправлениеОкнами проверяют активное окно, позицию и максимизацию,
# а без WM (голый Xvfb) эти свойства не работают.
log "Xvfb :${DISPLAY_NUM} + openbox"
Xvfb ":${DISPLAY_NUM}" -screen 0 1680x1050x24 -shmem >/dev/null 2>&1 &
sleep 2
openbox --sm-disable >/dev/null 2>&1 &
sleep 2
xwininfo -root -tree >/dev/null 2>&1 || log "предупреждение: X-сервер не отвечает"

# GTK/WebKit в headless-окружении: шина доступности и compositing не нужны,
# а без них клиент иногда встаёт на инициализации, не показав главное окно.
export NO_AT_BRIDGE=1
export WEBKIT_DISABLE_COMPOSITING_MODE=1
export GDK_BACKEND=x11

# --- 3a. Защита от опасных действий --------------------------------------
# Ничего не настраиваем: диалог «Разрешить открывать данный файл?» снимается
# программно при загрузке обработки — форма ОткрытьТестируемуОбработку создаёт
# её через ВнешниеОбработки.Создать с ОписаниеЗащитыОтОпасныхДействий
# (ПредупреждатьОбОпасныхДействиях = Ложь), поэтому подтверждать нечего и
# conf.cfg с DisableUnsafeActionProtection не нужен.
# Трогать conf.cfg нельзя: запись в /opt/1cv8t/conf/conf.cfg роняла
# LoadConfigFromFiles — платформа не принимала конфигурацию ещё до запуска
# клиента (прогон 41500778578 упал на шаге «База и .epf»).

# --- 4. Макеты внешней обработки -----------------------------------------
log "Макеты"
[ -n "${ADDIN_ZIP:-}" ] || fail "не задан ADDIN_ZIP"
[ -f "${ADDIN_ZIP}" ] || fail "не найден ${ADDIN_ZIP}"

mkdir -p "Example/Templates/${PROJECT}/Ext" "Example/Templates/Keywords/Ext"
cp "${ADDIN_ZIP}" "Example/Templates/${PROJECT}/Ext/Template.bin"
rm -f keywords.zip
zip -q -X -j keywords.zip keywords.json
cp keywords.zip "Example/Templates/Keywords/Ext/Template.bin"

# Библиотеку распаковываем рядом с обработкой и отдаём форме её путь.
# Подключение из бандла (макета) под Linux виснет: платформа не доводит
# установку до ExtCompT/registry.xml и не вызывает оповещение ни для адреса
# хранилища, ни для пути к zip — клиент стоит до таймаута. Прямой путь к .so
# разбор бандла не требует, поэтому форма AutotestRun предпочитает его, если
# рядом с .epf лежит addin_path.txt.
log "Распаковка библиотеки"
# Распаковываем и в подкаталог addin, и в корень: форма ищет библиотеку по
# маске сначала в addin/, потом рядом с обработкой. Оба варианта дешёвые, а
# маска в корне дополнительно страхует от расхождения в структуре архива.
rm -rf addin && mkdir -p addin
unzip -o -q "${ADDIN_ZIP}" -d addin
unzip -o -q "${ADDIN_ZIP}" -d .
# -quit вместо "| head -n 1": под pipefail head закрывает поток, find получает
# SIGPIPE, и весь скрипт падал бы на пустом месте.
so=$(find addin -maxdepth 1 -name '*Lin64*.so' -print -quit)
[ -n "${so}" ] || so=$(find addin -maxdepth 1 -name '*Lin32*.so' -print -quit)
[ -n "${so}" ] || fail "в ${ADDIN_ZIP} нет Linux-библиотеки (Lin64/Lin32)"
# Путь абсолютный: клиент запускается с другим рабочим каталогом.
printf '%s' "$(cd "$(dirname "${so}")" && pwd)/$(basename "${so}")" > addin_path.txt
log "библиотека: $(cat addin_path.txt)"
ls -la addin

# Зависимости библиотеки: если dlopen не сможет их разрешить, платформа
# отчитается о неудаче подключения без подробностей. ldd показывает причину
# сразу — «not found» рядом с конкретной библиотекой.
log "Зависимости библиотеки"
ldd "${so}" 2>&1 | tee addin-ldd.txt || true
if grep -q 'not found' addin-ldd.txt; then
	echo "::warning::у ${so} не разрешаются зависимости (см. ldd выше)"
fi

# --- 5. База, конфигурация и .epf ----------------------------------------
log "База и .epf"
rm -rf "${DB}" "${PROJECT}.epf"
mkdir -p "${DB}"

# Каждый вызов конфигуратора под timeout и со своим логом. Раньше вывод уходил
# в /dev/null, а fail сообщал только имя команды: по прогону 41500778578 было
# видно «LoadConfigFromFiles», но не причину, и разбор требовал нового прогона.
# timeout нужен отдельно: встретив диалог, конфигуратор стоит до таймаута шага.
run_designer() {
	local name="$1"
	shift
	local log_file="designer-${name}.log"
	set +e
	timeout -k 10 420 "$@" > "${log_file}" 2>&1
	local rc=$?
	set -e
	log "  ${name}: rc=${rc}, лог ${log_file}"
	if [ "${rc}" -ne 0 ] || [ -s "${log_file}" ]; then
		tail -n 20 "${log_file}" 2>/dev/null || true
	fi
	[ "${rc}" -eq 0 ] || fail "${name} (rc=${rc}, см. ${log_file})"
}

run_designer CREATEINFOBASE "${bin}" CREATEINFOBASE "File=${DB}" \
	/DisableStartupDialogs /DisableStartupMessages
run_designer LoadConfigFromFiles "${bin}" DESIGNER "/F${DB}" \
	/LoadConfigFromFiles "${WORKDIR}/Config" /UpdateDBCfg \
	/DisableStartupDialogs /DisableStartupMessages
run_designer LoadExternalDataProcessor "${bin}" DESIGNER "/F${DB}" \
	/LoadExternalDataProcessorOrReportFromFiles "${WORKDIR}/Example.xml" "${WORKDIR}/${PROJECT}.epf" \
	/Out example.log /DisableStartupDialogs /DisableStartupMessages
[ -f "${PROJECT}.epf" ] || fail "${PROJECT}.epf не создан"

# --- 6. Приёмник результатов и прогон ------------------------------------
# Форма AutotestRun читает app_port.txt и шлёт результаты в API AppVeyor.
# На раннере API нет — поднимаем локальную заглушку, код 1С не меняем.
log "Приёмник результатов тестов"
rm -f app_port.txt success.txt
python3 scripts/ci-appveyor-stub.py > stub.log 2>&1 &
stub_pid=$!

cleanup() {
	kill "${stub_pid}" 2>/dev/null || true
	pkill -f 1cv8 2>/dev/null || true
}
trap cleanup EXIT

for _ in $(seq 1 30); do
	[ -f app_port.txt ] && break
	sleep 1
done
[ -f app_port.txt ] || { cat stub.log; fail "приёмник не поднялся"; }
log "порт приёмника: $(cat app_port.txt)"

# Фаза prepare: заводит пользователя CI и отключает в самой базе предупреждения
# о загрузке внешнего кода. Без неё любой запуск внешней обработки показывает
# модальный диалог «Предупреждение безопасности» — в журнале регистрации это
# _$Session$_.ExternalDataProcessorConnectError, а в headless-сессии диалог
# некому закрыть, и клиент висит до таймаута, не открыв ни одной формы.
log "Подготовка базы (пользователь CI, без предупреждений)"
set +e
timeout -k 10 300 "${client}" ENTERPRISE "/F${DB}" \
	/DisableStartupDialogs /DisableStartupMessages \
	/Out prepare.out /C prepare
prepare_rc=$?
set -e
log "prepare: rc=${prepare_rc}"
sed -n '1,20p' prepare.out 2>/dev/null || true

log "Автотест в тонком клиенте"
set +e
# Обработку НЕ передаём через /Execute: при /Execute платформа показывает
# диалог «Предупреждение безопасности» до открытия формы, и в headless-сессии
# клиент встаёт на нём навсегда (проверено журналом регистрации базы). Вместо
# этого модуль приложения по /C сам открывает обработку через
# ВнешниеОбработки.Создать с отключёнными предупреждениями. /C идёт последним:
# остаток командной строки 1С считает его значением. Запуск под /N CI — под
# этим пользователем предупреждения выключены ещё и в самой базе.
client_args=(ENTERPRISE /LRU "/F${DB}" /N CI
	/DisableStartupDialogs /DisableStartupMessages /Out autotest.out /C autotest)
# GTK/WebKit нужна сессионная шина D-Bus: без неё старт клиента может
# зависнуть на инициализации, так и не создав главное окно.
if command -v dbus-run-session >/dev/null 2>&1; then
	run_prefix=(dbus-run-session --)
else
	log "предупреждение: dbus-run-session не найден, запускаю напрямую"
	run_prefix=()
fi
# 900 с, а не 300: полный набор тестов включает проверки управления окнами и
# на медленном раннере не укладывается в пять минут — таймаут обрывал прогон
# уже после успешного подключения компоненты (rc=124 при живом клиенте).
"${run_prefix[@]}" timeout -k 10 900 "${client}" "${client_args[@]}" > autotest.log 2>&1 &
client_pid=$!

# Пока клиент работает, раз в 30 с печатаем дерево окон. Если он встал на
# модальном диалоге (предупреждение о безопасности, выбор базы, ошибка
# подключения), это видно сразу, а не через 15 минут по таймауту.
waited=0
while kill -0 "${client_pid}" 2>/dev/null; do
	if [ -f success.txt ]; then
		break
	fi
	sleep 10
	waited=$((waited + 10))
	if [ $((waited % 30)) -eq 0 ]; then
		# Одна строка на итерацию: подробности печатаются один раз при падении.
		# Иначе десять итераций по 60 строк вытесняют из лога шага сам дамп.
		windows=$(xwininfo -root -tree 2>/dev/null | wc -l)
		cpu=$(ps -o pcpu= -C "$(basename "${client}")" 2>/dev/null | tr -d ' ' | head -n 1)
		log "ожидание ${waited} с: строк в дереве окон ${windows}, cpu ${cpu:-?}, autotest.out $(wc -c < autotest.out 2>/dev/null || echo 0) байт, trace: $(tail -n 1 trace.log 2>/dev/null || echo нет)"
	fi
done
wait "${client_pid}"
rc=$?
set -e
log "код возврата клиента: ${rc}"

if [ ! -f success.txt ]; then
	# Сводка в аннотацию: логи шага и артефакты доступны только владельцу
	# репозитория, а аннотации check-run отдаются публичным API. Без этого
	# причина падения видна только владельцу, и разбор затягивается.
	{
		echo "linux-test: тесты не прошли (rc=${rc})"
		echo "epf: $(ls -la "${PROJECT}.epf" 2>/dev/null | awk '{print $5" байт"}')"
		echo "autotest.out: $(wc -c < autotest.out 2>/dev/null || echo 0) байт"
		echo "--- trace.log ---"
		cat trace.log 2>/dev/null || echo 'нет trace.log: форма AutotestRun не открывалась'
		echo "--- autotest.log (хвост) ---"
		tail -n 15 autotest.log 2>/dev/null || echo пусто
		echo "--- autotest.out ---"
		tail -n 15 autotest.out 2>/dev/null || echo пусто
		echo "--- логи платформы (хвост) ---"
		find "${HOME}/.1cv8t/1C/1cv8t/logs" -name '*.log' -type f 2>/dev/null |
			while read -r f; do tail -n 12 "$f"; done | tail -n 30 || echo пусто
		echo "--- неразрешённые зависимости ---"
		grep -E 'not found' addin-ldd.txt 2>/dev/null || echo нет
		echo "--- registry.xml ---"
		head -c 200 "${HOME}/.1cv8t/1C/1cv8t/ExtCompT/registry.xml" 2>/dev/null || echo нет
	} > ci-failure-report.txt 2>&1
	annotate error "$(cat ci-failure-report.txt)"
	echo '--- сводка для аннотации ---'
	cat ci-failure-report.txt
	echo "--- код возврата клиента: ${rc} (124 = сработал timeout, значит завис) ---"
	echo '--- размеры логов ---'
	wc -c autotest.log autotest.out 2>/dev/null || true
	echo '--- начало вывода клиента ---'
	head -n 40 autotest.log 2>/dev/null || true
	echo '--- конец вывода клиента ---'
	tail -n 60 autotest.log 2>/dev/null || true
	echo '--- autotest.out ---'
	tail -n 20 autotest.out 2>/dev/null || true
	echo '--- маркеры выполнения (trace.log) ---'
	cat trace.log 2>/dev/null || echo 'нет trace.log'
	# Лог самой платформы: клиент держит его открытым, но в консоль он не попадает.
	# Копируем целиком в артефакты и печатаем хвосты.
	mkdir -p platform-logs
	cp -r "${HOME}"/.1cv8t/1C/1cv8t/logs/* platform-logs/ 2>/dev/null || true
	echo '--- логи платформы ---'
	find platform-logs -name '*.log' -type f 2>/dev/null | while read -r platform_log; do
		echo "=== ${platform_log} ==="
		tail -n 30 "${platform_log}" 2>/dev/null || true
	done | head -n 200
	echo '--- отчёт приёмника ---'
	cat tests-report.log 2>/dev/null || echo 'нет tests-report.log'
	echo '--- заглушка AppVeyor ---'
	cat stub.log 2>/dev/null || true
	echo "--- порт приёмника: $(cat app_port.txt 2>/dev/null || echo нет) ---"
	echo '--- база и обработка ---'
	ls -la "${DB}" 2>/dev/null | head -n 10 || true
	ls -la "${PROJECT}.epf" 2>/dev/null || true
	# Дерево окон целиком: в нём 200+ строк, и именно в хвосте (а не в первых
	# строках) виден модальный диалог, на котором клиент встал.
	echo '--- окна X (полное дерево) ---'
	xwininfo -root -tree 2>/dev/null || true
	echo '--- домашний каталог 1С ---'
	find "${HOME}/.1cv8t" -maxdepth 4 2>/dev/null | head -n 50 || true
	# ExtCompT/registry.xml — реестр внешних компонент: видно, дошла ли платформа
	# до установки компоненты из макета и что она про неё записала.
	echo '--- реестр внешних компонент ---'
	cat "${HOME}/.1cv8t/1C/1cv8t/ExtCompT/registry.xml" 2>/dev/null || echo 'нет registry.xml'
	echo '--- example.log (сборка .epf) ---'
	cat example.log 2>/dev/null || echo 'нет example.log'
	echo '--- журнал регистрации базы (текстовые строки) ---'
	for lgf in "${DB}"/1Cv8Log/*.lgp "${DB}"/1Cv8Log/*.lgf; do
		[ -f "${lgf}" ] || continue
		echo "=== ${lgf} ==="
		strings "${lgf}" 2>/dev/null | tail -n 40 || true
	done
	fail "автотест не дошёл до конца: success.txt не создан"
fi

log "Автотест пройден"
annotate notice "linux-test: тесты пройдены, библиотека $(cat addin_path.txt 2>/dev/null)"
cat tests-report.log 2>/dev/null || true
