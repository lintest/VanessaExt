## GitFor1C - реализация базового функционала Git

Предназначена для Windows и Linux. Разработана по технологии Native API в составе библиотеки VanessaExt.
Использует библиотеки [libgit2](https://libgit2.org/) и [libssh2](https://libssh2.org/).


### Подключение внешней компоненты

```bsl
&НаКлиенте
Перем git;

&НаКлиенте
Процедура ПодключениеВнешнейКомпонентыЗавершение(Подключение, ДополнительныеПараметры) Экспорт
	git = Новый("AddIn." + ИдентификаторКомпоненты + ".GitFor1C");
КонецПроцедуры	

```
### Свойства
- <a href="#Branches">Branches</a>
- <a href="#Head">Head</a>
- <a href="#Remotes">Remotes</a>
- <a href="#Signature">Signature</a>
- <a href="#Version">Version</a>

### Методы
Базовые методы
- <a href="#Find">Find</a>
- <a href="#Init">Init</a>
- <a href="#Open">Open</a>
- <a href="#Clone">Clone</a>
- <a href="#Close">Close</a>
- <a href="#Status">Status</a>
- <a href="#Commit">Commit</a>
- <a href="#History">History</a>
- <a href="#Tree">Tree</a>
- <a href="#Info">Info</a>
- <a href="#Diff">Diff</a>

Дополнительные методы
- <a href="#SetAuthor">SetAuthor</a>
- <a href="#SetCommitter">SetCommitter</a>

## Свойства
### <a name="Remotes">Remotes</a>
Тип значения: Строка (только чтение)
- Возвращает строку с текстом в формате JSON, при чтении которого получаем
массив объектов типа ***Структура*** с информациях об удалённых репозиториях.

Соответствует команде:
```sh
git remote -v
```

## Методы
### <a name="Find">Find(path)</a>
Параметры функции:
- **path** (обязательный), Тип: Строка

Тип возвращаемого значения: Строка
- Возвращает путь корневого каталога репозитория

### <a name="SetAuthor">SetAuthor(name, email)</a>
Параметры функции:
- **name** (обязательный), Тип: Строка
- **email** (обязательный), Тип: Строка

Возвращаемое значение отсутствует.

```bsl
git.SetAuthor("User Name", "user@mail.com");
```

### <a name="SetCommitter">SetCommitter(name, email)</a>
Параметры функции:
- **name** (обязательный), Тип: Строка
- **email** (обязательный), Тип: Строка

Возвращаемое значение отсутствует.
```bsl
git.SetCommitter("User Name", "user@mail.com");
```

