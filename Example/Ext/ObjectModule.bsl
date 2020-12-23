﻿Перем Ожидается;

#Область НаборТестов

Процедура ЗаполнитьНаборТестов(ЮнитТест) Экспорт
	
	Ожидается = ЮнитТест;
	ЮнитТест.Добавить("Тест_ИнициализацияБиблиотеки", "Инициализация библиотеки");
	ЮнитТест.Добавить("Тест_СвойстваУправлениеОкнами", "Свойства WindowsControl");
	ЮнитТест.Добавить("Тест_МетодыУправлениеОкнами", "Методы WindowsControl");
	ЮнитТест.Добавить("Тест_ПолучениеСнимкаЭкрана", "Получение снимка экрана");
	ЮнитТест.Добавить("Тест_ФункционалGit", "Тест функционала GIT");
	
КонецПроцедуры

Процедура Тест_ИнициализацияБиблиотеки() Экспорт
	
	ИдКомпоненты = "VanessaExt";
	МакетКомпоненты = ПолучитьМакет(ИдКомпоненты);
	АдресКомпоненты = ПоместитьВоВременноеХранилище(МакетКомпоненты);
	
	Ожидается.Тест("Подключение бибилиотеки компонент");
	Подключено = ПодключитьВнешнююКомпоненту(АдресКомпоненты, ИдКомпоненты, ТипВнешнейКомпоненты.Native);
	Ожидается.Что(Подключено).ЕстьИстина();
	
	ИменаКомпонент = Новый Структура("WindowsControl,ProcessControl,ClipboardControl,GitFor1C");
	Для каждого КлючЗначение из ИменаКомпонент Цикл
		Ожидается.Тест("Создание объкта: " + КлючЗначение.Ключ);
		ИмяКомпоненты = "AddIn." + ИдКомпоненты + "." + КлючЗначение.Ключ;
		Ожидается.Что(Новый(ИмяКомпоненты)).ИмеетТип(ИмяКомпоненты);
	КонецЦикла;
	
КонецПроцедуры

Процедура Тест_СвойстваУправлениеОкнами() Экспорт
	
	ВК = Новый("AddIn.VanessaExt.WindowsControl");
	
	Ожидается.Тест().Что(ВК).Получить("Version").ИмеетТип("Строка");
	Ожидается.Тест().Что(ВК).Получить("Версия").ИмеетТип("Строка");
	Ожидается.Тест().Что(ВК).Получить("ВЕРСИЯ").ИмеетТип("Строка");
	Ожидается.Тест().Что(ВК).Получить("ProcessId").Больше(0);
	Ожидается.Тест().Что(ВК).Получить("ИдентификаторПроцесса").Больше(0);
	Ожидается.Тест().Что(ВК).Получить("АктивноеОкно").Больше(0);
	
	ИмяСвойства = "ТекстБуфераОбмена";
	ТестовыеДанные = "Текст примера для проверки";
	Ожидается.Тест().Что(ВК).Установить(ИмяСвойства, ТестовыеДанные);
	Ожидается.Тест().Получить(ИмяСвойства).Равно(ТестовыеДанные);
	
	ИмяСвойства = "КартинкаБуфераОбмена";
	ТестовыеДанные = ПолучитьЛоготип1С();
	Ожидается.Тест().Что(ВК).Установить(ИмяСвойства, ТестовыеДанные);
	Ожидается.Тест().Получить(ИмяСвойства).ЭтоКартинка();
	
	Ожидается.Тест().Что(ВК).Получить("СвойстваЭкрана");
	Ожидается.Тест().Что(ВК).Получить("СписокДисплеев");
	Ожидается.Тест().Что(ВК).Получить("СписокОкон");
	Ожидается.Тест().Что(ВК).Получить("ПозицияКурсора");
	
КонецПроцедуры

Процедура Тест_МетодыУправлениеОкнами() Экспорт
	
	ВК = Новый("AddIn.VanessaExt.WindowsControl");
	
	Ожидается.Тест().Что(ВК).Функц("НайтиКлиентТестирования", 48000);
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСписокПроцессов", Истина);
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСписокДисплеев");
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСвойстваОкна", 0);
	
	ИдентификаторОкна = Ожидается.Тест().Что(ВК).Получить("АктивноеОкно").Значение();
	ИдентификаторПроцесса = Ожидается.Тест().Что(ВК).Получить("ИдентификаторПроцесса").Значение();
	СвойстваОкна = Ожидается.Тест().Что(ВК).Функц("ПолучитьСвойстваОкна", ИдентификаторОкна).ЗначениеJSON();
	Ожидается.Тест("Проверка идентификатора процесса").Что(СвойстваОкна).Получить("ProcessId").Равно(ИдентификаторПроцесса);
	
	
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСписокДисплеев", ИдентификаторОкна);
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСвойстваДисплея", ИдентификаторОкна);
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСвойстваОкна", ИдентификаторОкна);
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСписокОкон", ИдентификаторПроцесса);
	
	Размеры = Ожидается.Тест().Что(ВК).Функц("ПолучитьРазмерОкна", ИдентификаторОкна).ЗначениеJSON();
	Ожидается.Тест().Что(ВК).Проц("АктивироватьОкно", ИдентификаторПроцесса);
	Ожидается.Тест().Что(ВК).Проц("РаспахнутьОкно", ИдентификаторПроцесса);
	Ожидается.Тест().Что(ВК).Проц("РазвернутьОкно", ИдентификаторПроцесса);
	Ожидается.Тест().Что(ВК).Проц("СвернутьОкно", ИдентификаторПроцесса);
	Ожидается.Тест().Что(ВК).Проц("Пауза", 100);
	Ожидается.Тест().Что(ВК).Проц("РазвернутьОкно", ИдентификаторПроцесса);
	
	Ожидается.Тест().Что(ВК).Проц("УстановитьРазмерОкна", ИдентификаторОкна, Размеры.Width - 1, Размеры.Height - 1);
	Ожидается.Тест().Что(ВК).Проц("УстановитьПозициюОкна", ИдентификаторОкна, Размеры.Left + 1, Размеры.Top + 1);
	НовыеРазмеры = Ожидается.Тест().Что(ВК).Функц("ПолучитьРазмерОкна", ИдентификаторОкна).ЗначениеJSON();
	
	РазмерыСовпали = Истина
		И (Размеры.Left + 1 = НовыеРазмеры.Left) 
		И (Размеры.Top + 1 = НовыеРазмеры.Top) 
		И (Размеры.Width - 1 = НовыеРазмеры.Width) 
		И (Размеры.Height - 1 = НовыеРазмеры.Height)
		И (Размеры.Right = НовыеРазмеры.Right) 
		И (Размеры.Bottom = НовыеРазмеры.Bottom);
	
	Ожидается.Тест("Размеры и позиция окна совпали").Что(РазмерыСовпали).ЕстьИстина();
	
	Текст = Символы.ПС + "Пример вывода текста на консоль" + Символы.ПС;
	Ожидается.Тест().Что(ВК).Функц("ВывестиНаКонсоль", Текст).ЕстьИстина();
	
КонецПроцедуры

Процедура Тест_ПолучениеСнимкаЭкрана() Экспорт
	
	ВК = Новый("AddIn.VanessaExt.WindowsControl");
	
	ИдентификаторОкна = Ожидается.Тест().Что(ВК).Получить("АктивноеОкно").Значение();
	Размеры = Ожидается.Тест().Что(ВК).Функц("ПолучитьРазмерОкна", ИдентификаторОкна).ЗначениеJSON();
	
	СнимокОбласти = Ожидается.Тест().Что(ВК).Функц("ПолучитьСнимокОбласти", Размеры.Left, Размеры.Top, Размеры.Width, Размеры.Height).ЭтоКартинка().Значение();
	СнимокЭкрана = Ожидается.Тест().Что(ВК).Функц("ПолучитьСнимокЭкрана").ЭтоКартинка().Значение();
	СнимокОкна = Ожидается.Тест().Что(ВК).Функц("ПолучитьСнимокЭкрана", 1).ЭтоКартинка().Значение();
	
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСнимокОкна", ИдентификаторОкна).ЭтоКартинка();
	Ожидается.Тест().Что(ВК).Функц("ПолучитьСнимокОкна").ЭтоКартинка();
	
	Координаты = Ожидается.Тест().Что(ВК).Функц("НайтиФрагмент", СнимокЭкрана, СнимокОбласти).ЗначениеJSON();
	Ожидается.Тест("Фрагмент найден на картинке").Что(Координаты).Получить("Match").Больше(0.9);
	Координаты = Ожидается.Тест().Что(ВК).Функц("НайтиНаЭкране", СнимокОбласти).ЗначениеJSON();
	Ожидается.Тест("Фрагмент найден на экране").Что(Координаты).Получить("Match").Больше(0.9);
	
КонецПроцедуры

Процедура Тест_ФункционалGit() Экспорт
	
	ВК = Новый("AddIn.VanessaExt.GitFor1C");
	
	ИмяПапки = "Autotest";
	ВременнаяПапка = ПолучитьИмяВременногоФайла("git");
	УдалитьФайлы(ВременнаяПапка);
	СоздатьКаталог(ВременнаяПапка);
	
	Репозиторий = ВременнаяПапка + "/" + ИмяПапки + "/";
	Подкаталог = Репозиторий + "test";
	СоздатьКаталог(Репозиторий);
	СоздатьКаталог(Подкаталог);
	
	АдресКлон = "https://github.com/lintest/AddinTemplate.git";
	ПапкаКлон = ВременнаяПапка + "clone";
	ФайлКлон = ПапкаКлон + "/LICENSE";
	
	ИмяФайла = "пример.txt";
	ПолноеИмя = Репозиторий + ИмяФайла;
	ТекстовыйДокумент = Новый ТекстовыйДокумент;
	ТекстовыйДокумент.ДобавитьСтроку(ЧислоПрописью(51243, "L=en_US"));
	ТекстовыйДокумент.ДобавитьСтроку(ЧислоПрописью(24565, "L=en_US"));
	ТекстовыйДокумент.Записать(ПолноеИмя, КодировкаТекста.UTF8);
	
	Ожидается.Тест().Что(ВК).Получить("Version").ИмеетТип("Строка");
	Ожидается.Тест().Что(ВК).Проц("SetAuthor", "Автор", "author@lintest.ru");
	Ожидается.Тест().Что(ВК).Проц("SetCommitter", "Комиттер", "committer@lintest.ru");
	
	Ожидается.Тест("Клонирование репозитория").Что(ВК).Функц("clone", АдресКлон, ПапкаКлон).Успешно();
	Ожидается.Тест("Проверка сущестования файла").Что(ФайлСуществует(ФайлКлон)).ЕстьИстина();
	Ожидается.Тест("Информация о коммите").Что(ВК).Функц("info", "HEAD").Успешно();
	Репозитории = Ожидается.Тест().Что(ВК).Получить("remotes").Результат().ИмеетТип("Массив").Значение();
	Ожидается.Тест("Адрес репозитория").Что(Репозитории).Получить(0).Получить("url").Равно(АдресКлон);
	Ожидается.Тест("Закрытие репозитория").Что(ВК).Функц("close").Успешно();
	
	Ожидается.Тест("Инициализация репозитория").Что(ВК).Функц("init", Репозиторий).Успешно();
	Ожидается.Тест("Статус рабочей области").Что(ВК).Функц("status").Результат().Получить("work").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Добавление в индекс").Что(ВК).Функц("add", ИмяФайла).Успешно();
	Ожидается.Тест("Статус после добавления").Что(ВК).Функц("status").Результат().Получить("index").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Первый коммит").Что(ВК).Функц("commit", "Инициализация").Успешно();
	Коммит = Ожидается.Тест("Информация о коммите").Что(ВК).Функц("info", "HEAD").Результат().Значение();
	Ожидается.Тест("Автор коммита").Что(Коммит).Получить("authorName").Равно("Автор");
	Ожидается.Тест("Комиттер коммита").Что(Коммит).Получить("committerName").Равно("Комиттер");
	Ожидается.Тест("Закрытие репозитория").Что(ВК).Функц("close").Успешно();
	
	Статус = Ожидается.Тест("Статус закрытого репозитория").Что(ВК).Функц("status").ЗначениеJSON();
	Ожидается.Тест("Статус получить не удалось").Что(Статус).Получить("success").Равно(Ложь);
	Ожидается.Тест("Код ошибки равен нулю").Что(Статус).Получить("error").Получить("code").Равно(0);

	ИмяФайла = "текст.txt";
	ПолноеИмя = Репозиторий + ИмяФайла;
	ТекстовыйДокумент = Новый ТекстовыйДокумент;
	ТекстовыйДокумент.ДобавитьСтроку("Содержимое");
	ТекстовыйДокумент.Записать(ПолноеИмя, КодировкаТекста.UTF8);
	
	Статус = Ожидается.Тест("Поиск репозитория").Что(ВК).Функц("find", Подкаталог).Успешно().ЗначениеJSON();
	Ожидается.Тест("Открытие репозитория").Что(ВК).Функц("open", Статус.result).Успешно();
	Ожидается.Тест("Статус рабочей области").Что(ВК).Функц("status").Результат().Получить("work").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Добавление в индекс").Что(ВК).Функц("add", ИмяФайла).Успешно();
	Ожидается.Тест("Статус после добавления").Что(ВК).Функц("status").Результат().Получить("index").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Второй коммит").Что(ВК).Функц("commit", "Второй коммит").Успешно();
	Ожидается.Тест("Получение истории").Что(ВК).Функц("history").Результат().ИмеетТип("Массив").Функц("Количество").Равно(2);
	Ожидается.Тест("Создание новой ветки").Что(ВК).Функц("checkout", "develop", Истина).Успешно();
	Ожидается.Тест("Список веток").Что(ВК).Получить("branches").Результат().ИмеетТип("Массив").Функц("Количество").Равно(2);
	
	ТекстовыйДокумент = Новый ТекстовыйДокумент;
	ТекстовыйДокумент.ДобавитьСтроку("Редактирование");
	ТекстовыйДокумент.Записать(ПолноеИмя, КодировкаТекста.UTF8);
	
	Ожидается.Тест("Добавление в индекс").Что(ВК).Функц("add", ИмяФайла).Успешно();
	Ожидается.Тест("Статус после добавления").Что(ВК).Функц("status").Результат().Получить("index").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Удаление из индекса").Что(ВК).Функц("reset", ИмяФайла).Успешно();
	Ожидается.Тест("Статус после удаления").Что(ВК).Функц("status").Результат().Получить("work").Получить(0).Получить("new_name").Равно(ИмяФайла);
	Ожидается.Тест("Отмена изменений").Что(ВК).Функц("discard", ИмяФайла).Успешно();
	Ожидается.Тест("Статус после отмены").Что(ВК).Функц("status").Результат().Равно(Неопределено);
	
	ТекстовыйДокумент = Новый ТекстовыйДокумент;
	ТекстовыйДокумент.ДобавитьСтроку("Редактирование");
	ТекстовыйДокумент.Записать(ПолноеИмя, КодировкаТекста.UTF8);
	
	Ожидается.Тест("Добавление в индекс").Что(ВК).Функц("add", ИмяФайла).Успешно();
	Ожидается.Тест("Третий коммит").Что(ВК).Функц("commit", "Третий коммит").Успешно();
	Ожидается.Тест("Указатель HEAD").Что(ВК).Получить("head").Результат().Равно("refs/heads/develop");
	Ожидается.Тест("Переключение ветки").Что(ВК).Функц("checkout", "master").Успешно();
	Ожидается.Тест("Указатель HEAD").Что(ВК).Получить("head").Результат().Равно("refs/heads/master");
	
КонецПроцедуры

#КонецОбласти

#Область СлужебныеПроцедуры

Функция JSON(ТекстJSON)
	
	Если ПустаяСтрока(ТекстJSON) Тогда
		Возврат Неопределено;
	КонецЕсли;
	
	ПоляДаты = Новый Массив;
	ПоляДаты.Добавить("CreationDate");
	ПоляДаты.Добавить("date");
	
	ЧтениеJSON = Новый ЧтениеJSON();
	ЧтениеJSON.УстановитьСтроку(ТекстJSON);
	Возврат ПрочитатьJSON(ЧтениеJSON, , ПоляДаты);
	
КонецФункции

Функция ПолучитьЛоготип1С()
	
	ФайлРесурса = "v8res://mngbase/About.lf";
	ВременныйФайл = ПолучитьИмяВременногоФайла();
	КопироватьФайл(ФайлРесурса, ВременныйФайл);
	ТекстовыйДокумент = Новый ТекстовыйДокумент;
	ТекстовыйДокумент.Прочитать(ВременныйФайл);
	УдалитьФайлы(ВременныйФайл);
	
	Стр = ТекстовыйДокумент.ПолучитьТекст();
	НачПоз = СтрНайти(Стр, "{#base64:");
	КонПоз = СтрНайти(Стр, "}", , НачПоз);
	Стр = Сред(Стр, НачПоз + 9, КонПоз - НачПоз - 9);
	ДвоичныеДанные = Base64Значение(Стр);
	
	Картинка = Новый Картинка(ДвоичныеДанные);
	Соответствие = Новый Соответствие;
	Соответствие.Вставить("screenDensity", "xdpi");
	Возврат Картинка.ПолучитьДвоичныеДанные(Ложь, Соответствие);
	
КонецФункции

Функция ПрочитатьТекст(ДвоичныеДанные)
	
	Поток = ДвоичныеДанные.ОткрытьПотокДляЧтения();
	ЧтениеТекста = Новый ЧтениеТекста(Поток);
	Возврат СокрЛП(ЧтениеТекста.Прочитать());
	
КонецФункции	

Функция ФайлСуществует(ИмяФайла)
	
	Файл = Новый Файл(ИмяФайла);
	Возврат Файл.Существует();
	
КонецФункции	

#КонецОбласти
