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

cd "${WORKDIR}"

# --- 1. Зависимости -------------------------------------------------------
log "Зависимости"
sudo apt-get update -qq
sudo apt-get install -y -qq --no-install-recommends \
	p7zip-full xvfb x11-utils openbox \
	libwebkit2gtk-4.0-37 libjavascriptcoregtk-4.0-18 libsoup2.4-1 \
	libglu1-mesa libgtk-3-0 libxtst6 libxrender1 libxi6 libglib2.0-0 \
	libnss3 libxext6 libsm6 libice6 libfreetype6 libfontconfig1 \
	libdbus-1-3 libasound2 libxslt1.1 libxfixes3 libxcursor1 \
	libxrandr2 libcups2 libpango-1.0-0 libcairo2 libgbm1 libpng16-16 \
	locales fonts-dejavu-core zip

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

# --- 4. Макеты внешней обработки -----------------------------------------
log "Макеты"
[ -n "${ADDIN_ZIP:-}" ] || fail "не задан ADDIN_ZIP"
[ -f "${ADDIN_ZIP}" ] || fail "не найден ${ADDIN_ZIP}"

mkdir -p "Example/Templates/${PROJECT}/Ext" "Example/Templates/Keywords/Ext"
cp "${ADDIN_ZIP}" "Example/Templates/${PROJECT}/Ext/Template.bin"
rm -f keywords.zip
zip -q -X -j keywords.zip keywords.json
cp keywords.zip "Example/Templates/Keywords/Ext/Template.bin"

# --- 5. База, конфигурация и .epf ----------------------------------------
log "База и .epf"
rm -rf "${DB}" "${PROJECT}.epf"
mkdir -p "${DB}"
"${bin}" CREATEINFOBASE "File=${DB}" \
	/DisableStartupDialogs /DisableStartupMessages >/dev/null 2>&1 \
	|| fail "CREATEINFOBASE"
"${bin}" DESIGNER "/F${DB}" /LoadConfigFromFiles "${WORKDIR}/Config" /UpdateDBCfg \
	/DisableStartupDialogs /DisableStartupMessages >/dev/null 2>&1 \
	|| fail "LoadConfigFromFiles"
"${bin}" DESIGNER "/F${DB}" \
	/LoadExternalDataProcessorOrReportFromFiles "${WORKDIR}/Example.xml" "${WORKDIR}/${PROJECT}.epf" \
	/Out example.log /DisableStartupDialogs /DisableStartupMessages \
	|| fail "LoadExternalDataProcessorOrReportFromFiles (см. example.log)"
[ -f "${PROJECT}.epf" ] || fail "${PROJECT}.epf не создан"

# --- 6. Приёмник результатов и прогон ------------------------------------
# Форма AppVeyor читает app_port.txt и шлёт результаты в API AppVeyor.
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

log "Автотест в тонком клиенте"
set +e
# Тест запускается из конфигурации: клиент открывает
# Обработка.ОткрытьТестируемуОбработку.Форма, путь к внешней обработке
# приходит в параметре запуска. Защиту от опасных действий обработка
# снимает сама (ОписаниеЗащитыОтОпасныхДействий), поэтому conf.cfg не нужен.
timeout -k 10 300 "${client}" ENTERPRISE /LRU "/F${DB}" \
	"/C${WORKDIR}/${PROJECT}.epf" \
	/DisableStartupDialogs /DisableStartupMessages /Out autotest.out \
	> autotest.log 2>&1 &
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
		log "ожидание ${waited} с, строк в дереве окон: $(xwininfo -root -tree 2>/dev/null | wc -l)"
		xwininfo -root -tree 2>/dev/null | sed -n '1,25p' | sed 's/^/    /'
		echo '    --- autotest.out (журнал клиента) ---'
		tail -n 10 autotest.out 2>/dev/null | sed 's/^/    /'
		echo '    --- процесс клиента ---'
		ps -o pid,stat,pcpu,etime,args -C "$(basename "${client}")" 2>/dev/null | sed 's/^/    /'
	fi
done
wait "${client_pid}"
rc=$?
set -e
log "код возврата клиента: ${rc}"

if [ ! -f success.txt ]; then
	echo "--- код возврата клиента: ${rc} (124 = сработал timeout, значит завис) ---"
	echo '--- размеры логов ---'
	wc -c autotest.log autotest.out 2>/dev/null || true
	echo '--- начало вывода клиента ---'
	head -n 40 autotest.log 2>/dev/null || true
	echo '--- конец вывода клиента ---'
	tail -n 60 autotest.log 2>/dev/null || true
	echo '--- autotest.out ---'
	tail -n 20 autotest.out 2>/dev/null || true
	echo '--- отчёт приёмника ---'
	cat tests-report.log 2>/dev/null || echo 'нет tests-report.log'
	echo '--- заглушка AppVeyor ---'
	cat stub.log 2>/dev/null || true
	echo "--- порт приёмника: $(cat app_port.txt 2>/dev/null || echo нет) ---"
	echo '--- база и обработка ---'
	ls -la "${DB}" 2>/dev/null | head -n 10 || true
	ls -la "${PROJECT}.epf" 2>/dev/null || true
	echo '--- окна X ---'
	xwininfo -root -tree 2>/dev/null | head -n 40 || true
	fail "автотест не дошёл до конца: success.txt не создан"
fi

log "Автотест пройден"
cat tests-report.log 2>/dev/null || true
