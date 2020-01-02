# 1cWinCtrl - внешняя компонента 1С 

Предназначена для управления окнами Windows и Linux, разработана по технологии Native API.

### Возможности компоненты:
- Получение списка окон и списка процессов
- Управление размерами и положением окна
- Получение снимка окна и снимка экрана

### Свойства

- <a href="#ProcessId">ИдентификаторПроцесса (ProcessId)</a>
- <a href="#CurrentWindow">ТекущееОкно (CurrentWindow)</a>
- <a href="#ActiveWindow">АктивноеОкно (ActiveWindow)</a>
- <a href="#ScreenInfo">СвойстваЭкрана (ScreenInfo)</a>
- <a href="#DisplayList">СписокДисплеев (DisplayList)</a>
- <a href="#WindowList">СписокОкон (WindowList)</a>

### Методы

Работа с процессами:
- <a href="#FindTestClient">НайтиКлиентТестирования (FindTestClient)</a>
- <a href="#GetProcessList">ПолучитьСписокПроцессов (GetProcessList)</a>
- <a href="#GetProcessInfo">ПолучитьСвойстваПроцесса (GetProcessInfo)</a>

Информация об окнах и окружении:
- <a href="#GetDisplayList">ПолучитьСписокДисплеев (GetDisplayList)</a>
- <a href="#GetDisplayInfo">ПолучитьСвойстваДисплея (GetDisplayInfo)</a>
- <a href="#GetWindowList">ПолучитьСписокОкон (GetWindowList)</a>

Управление окном приложения:
- <a href="#GetWindowState">ПолучитьСтатусОкна (GetWindowState)</a>
- <a href="#SetWindowState">УстановитьСтатусОкна (SetWindowState)</a>
- <a href="#SetWindowSize">УстановитьРазмерОкна (SetWindowSize)</a>
- <a href="#SetWindowPos">УстановитьПозициюОкна (SetWindowPos)</a>
- <a href="#EnableResizing">РазрешитьИзменятьРазмер (EnableResizing)</a>
- <a href="#GetWindowText">ПолучитьЗаголовок (GetWindowText)</a>
- <a href="#SetWindowText">УстановитьЗаголовок (SetWindowText)</a>
- <a href="#SetWindowText">АктивироватьОкно (ActivateWindow)</a>
- <a href="#MaximixeWindow">РаспахнутьОкно (MaximixeWindow)</a>
- <a href="#RestoreWindow">РазвернутьОкно (RestoreWindow)</a>
- <a href="#MinimizeWindow">СвернутьОкно (MinimizeWindow)</a>

Захвата изображения экрана:
- <a href="#TakeScreenshot">ПолучитьСнимокЭкрана (TakeScreenshot)</a>
- <a href="#CaptureWindow">ПолучитьСнимокОкна (CaptureWindow)</a>

### Общая информация

Внешняя компонента поддерживает как синхронный, так и асинхронный вызов.
Для асинхронного вызова в полном соответствии с документацией Синтакс-помощника
1С:Предприятие применяются методы:
- НачатьВызов<ИмяМетода>(<ОписаниеОповещения>, <Параметры>)
- НачатьПолучение<ИмяСвойства>(<ОписаниеОповещения>)

Пример асинхронного вызова внешней компоненты:
```bsl
&НаКлиенте
Процедура НайтиКлиентТестирования(Команда)
	ОписаниеОповещения = Новый ОписаниеОповещения("ПолученКлиентТестирования", ЭтотОбъект);
	ВнешняяКомпонента.НачатьВызовНайтиКлиентТестирования(ОписаниеОповещения, ПортПодключения);
КонецПроцедуры

&НаКлиенте
Процедура ПолученКлиентТестирования(РезультатВызова, ПараметрыВызова, ДополнительныеПараметры) Экспорт
	ДескрипторОкна = РезультатВызова;
КонецПроцедуры	
```

Далее по тексту все примеры будут приводиться только для синхронных вызовов.
В тестовой обработке **Example.epf** используются только асинхронные вызовы.

Многие свойства и методы компоненты возвращают сложные типы данных, которые сериализованы 
в строку формата JSON. Поэтому имеет смысл объявить в вызывающем модуле универсальную 
функцию, которая будет использоваться ниже в примерах работы компоненты:
```bsl
Функция ПрочитатьСтрокуJSON(ТекстJSON)
	Если ПустаяСтрока(ТекстJSON) Тогда
		Возврат Неопределено;
	Иначе
		ЧтениеJSON = Новый ЧтениеJSON();
		ЧтениеJSON.УстановитьСтроку(ТекстJSON);
		Возврат ПрочитатьJSON(ЧтениеJSON);
	КонецЕсли;
КонецФункции
```
### Сборка проекта

Готовая сборка внешней компоненты находится в файле 
[/Example/Templates/SetWindow/Ext/Template.bin](https://github.com/lintest/1cWinCtrl/raw/master/Example/Templates/SetWindow/Ext/Template.bin)

Порядок самостоятельной сборки внешней компоненты из исходников:
1. Для сборки компоненты необходимо установить Visual Studio Community 2019.
2. Чтобы работала сборка примера обработки EPF надо установить OneScript версии 1.0.20 или выше.
3. Для запуска сборки из исходников надо запустить ./Compile.bat.

Сборка для Linux осуществляется в CentOS 8:
```bash
yum -y group install "Development Tools"
yum -y install cmake glibc-devel.i686 glibc-devel libuuid-devel 
yum -y install libstdc++-devel.i686 gtk2-devel.i686 glib2-devel.i686
yum -y install libstdc++-devel.x86_64 gtk2-devel.x86_64 glib2-devel.x86_64
```

***

## Свойства

### <a name="CurrentWindow">ТекущееОкно / CurrentWindow</a>
Тип значения: Целое число (только чтение)
- Дескриптор основного окна приложения 1С, в сеансе которого вызывается внешняя компонента

### <a name="ActiveWindow">АктивноеОкно / ActiveWindow</a>
Тип значения: Целое число (только чтение)
- Дескриптор приоритетного окна (окна, с которым пользователь в настоящее время работает). 

### <a name="ProcessId">ИдентификаторПроцесса / ProcessId</a>
Тип значения: Целое число (только чтение)
- Идентификатор основного процесса приложения 1С, в сеансе которого вызывается внешняя компонента

### <a name="DisplayList">СписокДисплеев / DisplayList</a>
Тип значения: Строка (только чтение)
- Содержит строку с текстом в формате JSON, при чтении которого получаем
объект типа ***Массив*** из элементов типа **Структура** с размерами дисплеев мониторов.

### <a name="WindowList">СписокОкон / WindowList</a>
Тип значения: Строка (только чтение)
- Содержит строку с текстом в формате JSON, при чтении которого получаем
объект типа ***Массив*** из элементов типа **Структура** с информацией об окнах 
верхнего уровня: дескриптор окна, диентификатор процесса, владелец, заголовок окна.

### <a name="ScreenInfo">СвойстваЭкрана / ScreenInfo</a>
Тип значения: Строка (только чтение)
- Содержит строку с текстом в формате JSON, при чтении которого получаем
объект типа ***Структура*** с размерами экрана и рабочей области.

## Методы
### <a name="FindTestClient">НайтиКлиентТестирования(НомерПорта) / FindTestClient</a>
Возвращает текст в формате JSON, при чтении которого получаем объект типа ***Структура***, 
сотдержащий информацию о клиенте тестирования, найденному по номеру порта, 
который присутствует в командной строке экземпляра клиента тестирования.

Параметры функции:
- **НомерПорта** (обязательный), Тип: Целое число

Тип возвращаемого значения: Строка
- Содержит строку с текстом в формате JSON, при чтении которого получаем
объект типа ***Структура*** с подробной информацией о найденном процессе.
    - ProcessId - идентификатор процесса (Число)
	- CommandLine - командная строка процесса (Строка)
	- CreationDate - дата старта процесса (Дата)
	- Window - дескриптор основного окна (Число)
	- Title - заголовок основного окна (Строка)

```bsl
ТекстJSON = ВнешняяКомпонента.НайтиКлиентТестирования(ПортПодключения);
СтруктураСвойствПроцесса = ПрочитатьСтрокуJSON(ТекстJSON);
Если СтруктураСвойствПроцесса <> Неопределено Тогда
	ДескрипторОкна = СтруктураСвойствПроцесса.Window;
	ИдентификаторПроцесса = СтруктураСвойствПроцесса.ProcessId;
КонецЕсли;
```

### <a name="GetProcessInfo">ПолучитьСвойстваПроцесса(ИдентификаторПроцесса) / GetProcessInfo</a>
По идентификатору процесса возвращает всю доступную информацию о процессе.

Параметры функции:
- **ИдентификаторПроцесса** (обязательный), Тип: Целое число

Тип возвращаемого значения: Строка
- Содержит строку с текстом в формате JSON, при чтении которого получаем
объект типа ***Структура*** с подробной информацией о процессе.

```bsl
ТекстJSON = ВнешняяКомпонента.ПолучитьСвойстваПроцесса(ИдентификаторПроцесса);
СтруктураСвойстваПроцесса = ПрочитатьСтрокуJSON(ТекстJSON);
```

### <a name="GetDisplayList">ПолучитьСписокДисплеев(ДескрипторОкна) / GetDisplayList</a>
По дескриптору окна получает список дисплеев, на которых располагается окно или его часть.

Параметры функции:
- **ДескрипторОкна** (необязательный), Тип: Целое число
Если параметр не задан, будет получена информация обо всех дисплеях.

Тип возвращаемого значения: Строка
- Содержит строку с текстом в формате JSON, при чтении которого получаем объект типа 
***Массив*** из элементов типа **Структура** со свойствами дисплея: координаты границ, 
высота и ширина, наименование дисплея, координаты и размер рабочей области дисплея.

```bsl
ТекстJSON = ВнешняяКомпонента.ПолучитьСписокДисплеев(ДескрипторОкна);
Для каждого ЭлементМассива из ПрочитатьСтрокуJSON(ТекстJSON) Цикл
	ЗаполнитьЗначенияСвойств(СписокДисплеев.Добавить(), ЭлементМассива);
КонецЦикла;
```

### <a name="GetDisplayInfo">ПолучитьСвойстваДисплея(ДескрипторОкна) / GetDisplayInfo</a>
По дескриптору окна получает свойства дисплея, на котором располагается наибольшая часть окна.

Параметры функции:
- **ДескрипторОкна** (необязательный), Тип: Целое число
Если параметр не задан, будет получена информация для активного окна.

Тип возвращаемого значения: Строка
- Содержит строку с текстом в формате JSON, при чтении которого получаем объект 
типа ***Структура*** со свойствами дисплея: координаты границ, высота и ширина,
наименование дисплея, координаты и размер рабочей области дисплея.

```bsl
ТекстJSON = ВнешняяКомпонента.ПолучитьСвойстваДисплея(ДескрипторОкна);
СвойстваДисплея = ПрочитатьСтрокуJSON(ТекстJSON);
ВнешняяКомпонента.УстановитьПозициюОкна(ДескрипторОкна, СвойстваДисплея.Left, СвойстваДисплея.Top);
```

### <a name="GetWindowList">ПолучитьСписокОкон(ИдентификаторПроцесса) / GetWindowList</a>
Возвращает текст в формате JSON, при чтении которого получаем объект объект типа ***Массив*** 
из элементов типа **Структура** с информацией об окнах, принадлежащих указанному процессу.

Параметры функции:
- **ИдентификаторПроцесса** (обязательный), Тип: Целое число
	- Если параметр нулевой или не задан, возвращается список всех окон.

Тип возвращаемого значения: Строка
- Содержит строку с текстом в формате JSON, при чтении которого объект типа ***Массив*** 
из элементов типа ***Структура*** с подробной информацией о найденых окнах.
    - ProcessId - идентификатор процесса (Число)
	- Window - дескриптор окна (Число)
	- Title - заголовок окна (Строка)
	- Class - системный класс окна (Строка)
	- Owner - окно владелец (Число)

```bsl
ТекстJSON = ВнешняяКомпонента.ПолучитьСписокОкон(ИдентификаторПроцесса);
МассивОкон = ПрочитатьСтрокуJSON(ТекстJSON);
ТаблицаОкон.Очистить();
Для каждого Стр из МассивОкон Цикл
	ЗаполнитьЗначенияСвойств(ТаблицаОкон.Добавить(), Стр);
КонецЦикла;
```

### <a name="GetWindowState">ПолучитьСтатусОкна(ДескрипторОкна) / GetWindowState</a>
По десприптору окна получает режим отображения: свёрнуто, развёрнуто, распахнуто.

Параметры функции:
- **ДескрипторОкна** (необязательный), Тип: Целое число
	- Если параметр не задан, возвращается статус активного окна.

Тип возвращаемого значения: Целое число
- 0 - Если окно свёрнуто (минимизировано)
- 1 - Если окно развёрнуто (нормальное)
- 2 - Если коно распахнуто (максимизировано)

```bsl
СтатусОкна = ВнешняяКомпонента.ПолучитьСтатусОкна(ДескрипторОкна);
```

### <a name="SetWindowState">УстановитьСтатусОкна(ДескрипторОкна, СтатусОкна, Активировать) / SetWindowState</a>
По десприптору окна устанавливает его режим отображения: свёрнуто, развёрнуто, распахнуто.

Параметры функции:
- **ДескрипторОкна** (необязательный), Тип: Целое число
Если параметр не задан, изменяется статус активного окна.

- **СтатусОкна** (необязательный), Тип: Целое число
    - 0 - Чтобы свернуть окно (минимизировать)
    - 1 - Чтобы развернуть окно (нормализовать), по умолчанию
    - 2 - Чтобы распахнуть окно (максимизировать)

- **Активировать** (необязательный), Тип: Булево
    - Параметр игнорируется операционной системой Windows.

```bsl
ВнешняяКомпонента.УстановитьСтатусОкна(0, 3, Истина);
```

### <a name="TakeScreenshot">ПолучитьСнимокЭкрана(Режим) / TakeScreenshot</a>
Получает снимок экрана или активного окна, в зависимости от переданного параметра.

Параметры функции:
- **Режим** (обязательный), Тип: Целое число
    - 0 - Чтобы сделать снимок всего экрана
    - 1 - Чтобы сделать снимок области активного окна

Тип возвращаемого значения: Двоичные данные
- Возвращает картинку в формате PNG.

```bsl
ДвоичныеДанные = ВнешняяКомпонента.ПолучитьСнимокЭкрана(0);
```

### <a name="CaptureWindow">ПолучитьСнимокОкна(ДескрипторОкна) / CaptureWindow</a>
Получает снимок произвольного окна по его дескриптору.

Параметры функции:
- **ДескрипторОкна** (обязательный), Тип: Целое число
    - 0 - Чтобы сделать снимок активного окна


Тип возвращаемого значения: Двоичные данные
- Возвращает картинку в формате PNG.

```bsl
ДвоичныеДанные = ВнешняяКомпонента.ПолучитьСнимокОкна(ДескрипторОкна);
```
