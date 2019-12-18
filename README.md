# 1cWinCtrl - внешняя компонента 1С 

Предназначена для управления окнами Windows

Возможности компоненты:
- Получение списка окон и списка процессов
- Управление размерами и положением окна
- Получение снимка окна

## Свойства

- ТекущееОкно (CurrentWindow)
- АктивноеОкно (ActiveWindow)
- ИдентификаторПроцесса (ProcessId)

## Методы

- ПолучитьСписокПроцессов (GetProcessList)
- ПолучитьДанныеПроцесса (GetProcessInfo)
- ПолучитьСписокОкон (GetWindowList)
- УстановитьРазмерОкна (SetWindowSize)
- УстановитьПозициюОкна (SetWindowPos)
- РазрешитьИзменятьРазмер (EnableResizing)
- ПолучитьСнимокЭкрана (TakeScreenshot)
- ПолучитьСнимокОкна (CaptureWindow)
- ПолучитьЗаголовок (GetWindowText)
- УстановитьЗаголовок (SetWindowText)
- АктивироватьОкно (ActivateWindow)
- РаспахнутьОкно (MaximixeWindow)
- РазвернутьОкно (RestoreWindow)
- СвернутьОкно (MinimizeWindow)
