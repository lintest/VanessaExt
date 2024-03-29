﻿
&НаКлиенте
Процедура ТестируемаяОбработкаНачалоВыбора(Элемент, ДанныеВыбора, СтандартнаяОбработка)
	
	ОписаниеОповещения = Новый ОписаниеОповещения("ОбработкаВыбораФайла", ЭтотОбъект, "ТестируемаяОбработка");
	ДиалогВыбораФайла = Новый ДиалогВыбораФайла(РежимДиалогаВыбораФайла.Открытие);
	ДиалогВыбораФайла.Фильтр = "Внешняя обработка (*.epf)|*.epf";
	ДиалогВыбораФайла.Показать(ОписаниеОповещения);
	
КонецПроцедуры

&НаКлиенте
Процедура ТестируемаяБиблиотекаНачалоВыбора(Элемент, ДанныеВыбора, СтандартнаяОбработка)
	
	СистемнаяИнформация = Новый СистемнаяИнформация;
	Если СистемнаяИнформация.ТипПлатформы = ТипПлатформы.Windows_x86 
		ИЛИ СистемнаяИнформация.ТипПлатформы = ТипПлатформы.Windows_x86_64 Тогда
		ФильтрФайловБиблиотеки = "Библиотека (*.dll)|*.dll";
	Иначе
		ФильтрФайловБиблиотеки = "Библиотека (*.so)|*.so";
	КонецЕсли;

	ОписаниеОповещения = Новый ОписаниеОповещения("ОбработкаВыбораФайла", ЭтотОбъект, "ТестируемаяБиблиотека");
	ДиалогВыбораФайла = Новый ДиалогВыбораФайла(РежимДиалогаВыбораФайла.Открытие);
	ДиалогВыбораФайла.Фильтр = ФильтрФайловБиблиотеки;
	ДиалогВыбораФайла.Показать(ОписаниеОповещения);
	
КонецПроцедуры

&НаКлиенте
Процедура ОбработкаВыбораФайла(ВыбранныеФайлы, ДополнительныеПараметры) Экспорт 

	Если ВыбранныеФайлы <> Неопределено Тогда 
		НаборКонстант[ДополнительныеПараметры] = ВыбранныеФайлы[0];
	КонецЕсли;
	
КонецПроцедуры
