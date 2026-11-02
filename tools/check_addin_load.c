/*
 * Смоук-тест загрузки нативной внешней компоненты без платформы 1С.
 *
 * Зачем: GUI-автотест в тонком клиенте 1С под Linux нестабилен (клиент виснет
 * на подключении компоненты), поэтому о том, загружается ли .so вообще, он
 * ничего сказать не может: и «библиотека не грузится», и «платформа зависла»
 * выглядят одинаково — rc=124 по таймауту. Здесь библиотека открывается тем же
 * способом, каким это делает платформа (dlopen), и в случае неудачи печатается
 * точный текст dlerror(), а не молчание.
 *
 * Проверяется:
 *   1. dlopen библиотеки (разрешение зависимостей и зависимых символов);
 *   2. наличие обязательных точек входа ABI: GetClassNames, GetClassObject,
 *      DestroyObject, GetAttachType, SetPlatformCapabilities;
 *   3. что GetClassNames возвращает непустой список имён классов.
 *
 * Сборка и запуск:
 *   cc -O1 -o check_addin_load tools/check_addin_load.c -ldl
 *   ./check_addin_load path/to/VanessaExtLin64_1-4-0-40.so
 *
 * Код возврата: 0 — библиотека загружается и отдаёт имена классов,
 * 1 — не загрузилась или не отдала имена (причина в stderr).
 *
 * ВАЖНО: порядок инициализации повторяет платформу. SetPlatformCapabilities
 * вызывается до GetClassObject, иначе компонента, рассчитанная на конкретную
 * версию ABI платформы, может отказаться работать.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

/* Значения перечисления AppCapabilities и AttachType из
 * include/ComponentBase.h. Проверяются на уровне ABI: если в библиотеке
 * другая раскладка enum, SetPlatformCapabilities вернёт неожиданное значение
 * и тест это покажет. eAppCapabilitiesLast = eAppCapabilities3 = 3. */
enum {
	eAppCapabilities3 = 3,
	eAppCapabilitiesLast = eAppCapabilities3,
	eCanAttachNotIsolated = 1
};

typedef const uint16_t* (*GetClassNamesFn)(void);
typedef long (*GetClassObjectFn)(const uint16_t* name, void** pInterface);
typedef long (*DestroyObjectFn)(void** pInterface);
typedef int (*GetAttachTypeFn)(void);
typedef int (*SetPlatformCapabilitiesFn)(int capabilities);

static void* must_sym(void* handle, const char* name, int* failed)
{
	void* sym = dlsym(handle, name);
	if (sym == NULL) {
		fprintf(stderr, "ОШИБКА: в библиотеке нет символа %s\n", name);
		*failed = 1;
	}
	return sym;
}

/* WCHAR_T на Linux — uint16_t, поэтому имена приходят в UTF-16LE. */
static void print_utf16(const char* label, const uint16_t* text)
{
	char buffer[1024];
	size_t length = 0;

	if (text == NULL) {
		printf("%s: (null)\n", label);
		return;
	}
	while (text[length] != 0 && length + 1 < sizeof(buffer)) {
		uint16_t ch = text[length];
		buffer[length] = (ch < 0x80) ? (char)ch : '?';
		length++;
	}
	buffer[length] = '\0';
	printf("%s: %s\n", label, buffer);
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "использование: %s <путь к .so>\n", argv[0]);
		return 1;
	}

	int failed = 0;
	const char* path = argv[1];

	/* RTLD_NOW, а не RTLD_LAZY: неразрешённые символы должны всплыть здесь,
	 * а не в момент первого вызова метода компоненты на клиенте. */
	void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (handle == NULL) {
		const char* error = dlerror();
		fprintf(stderr, "ОШИБКА: dlopen не смог загрузить %s\n", path);
		fprintf(stderr, "  dlerror: %s\n", error != NULL ? error : "(пусто)");
		return 1;
	}
	printf("dlopen: успех (%s)\n", path);

	SetPlatformCapabilitiesFn set_capabilities =
		(SetPlatformCapabilitiesFn)must_sym(handle, "SetPlatformCapabilities", &failed);
	GetAttachTypeFn get_attach_type =
		(GetAttachTypeFn)must_sym(handle, "GetAttachType", &failed);
	GetClassNamesFn get_class_names =
		(GetClassNamesFn)must_sym(handle, "GetClassNames", &failed);
	GetClassObjectFn get_class_object =
		(GetClassObjectFn)must_sym(handle, "GetClassObject", &failed);
	DestroyObjectFn destroy_object =
		(DestroyObjectFn)must_sym(handle, "DestroyObject", &failed);

	if (failed) {
		dlclose(handle);
		return 1;
	}

	/* Платформа вызывает это до создания объектов и ожидает, что компонента
	 * вернёт eAppCapabilitiesLast. Другое значение означает расхождение
	 * версий ABI — типичная причина «молчаливого» отказа подключения. */
	int capabilities = set_capabilities(eAppCapabilitiesLast);
	printf("SetPlatformCapabilities: вернул %d (ожидается %d)\n",
		capabilities, eAppCapabilitiesLast);
	if (capabilities != eAppCapabilitiesLast) {
		failed = 1;
	}

	/* eCanAttachNotIsolated (1): компонента хочет работать в процессе клиента,
	 * а не в изолированном. Именно так объявлен GetAttachType в AddInNative. */
	int attach_type = get_attach_type();
	printf("GetAttachType: %d (ожидается %d)\n", attach_type, eCanAttachNotIsolated);
	if (attach_type != eCanAttachNotIsolated) {
		fprintf(stderr, "ПРЕДУПРЕЖДЕНИЕ: GetAttachType вернул %d\n", attach_type);
	}

	/* Имена классов: пустая строка означает, что ни один компонент не
	 * зарегистрирован, и платформа не сможет создать объект по имени. */
	const uint16_t* names = get_class_names();
	print_utf16("GetClassNames", names);
	if (names == NULL || names[0] == 0) {
		fprintf(stderr, "ОШИБКА: список имён классов пуст\n");
		failed = 1;
	}

	/* Создание первого объекта по имени из списка. Конструктор компоненты
	 * регистрирует свойства и методы, поэтому здесь ловятся падения
	 * статических инициализаций, которые не видны на dlopen. */
	if (!failed) {
		char first[64];
		size_t length = 0;
		while (names[length] != 0 && names[length] != '|' && length + 1 < sizeof(first)) {
			first[length] = (names[length] < 0x80) ? (char)names[length] : '?';
			length++;
		}
		first[length] = '\0';
		printf("создание объекта: %s\n", first);

		/* Имя передаётся в UTF-16LE — так его ждёт GetClassObject. */
		uint16_t wide[64];
		size_t i = 0;
		for (; i < length; i++) {
			wide[i] = (uint16_t)(unsigned char)first[i];
		}
		wide[length] = 0;

		void* object = NULL;
		long created = get_class_object(wide, &object);
		printf("GetClassObject: код %ld, объект %s\n", created,
			object != NULL ? "создан" : "НЕ создан");
		if (object == NULL) {
			failed = 1;
		}
		else {
			if (destroy_object(&object) != 0) {
				fprintf(stderr, "ОШИБКА: DestroyObject вернул ошибку\n");
				failed = 1;
			}
			else {
				printf("DestroyObject: успех\n");
			}
		}
	}

	dlclose(handle);
	if (failed) {
		fprintf(stderr, "ИТОГ: библиотека загрузилась, но проверки ABI не пройдены\n");
		return 1;
	}
	printf("ИТОГ: библиотека загружается и отдаёт объекты\n");
	return 0;
}
