import re
import sys

# Проверки на конструкции, которые 1С не компилирует, но легко написать по
# привычке из других языков.
p = sys.argv[1]
lines = open(p, encoding='utf-8-sig').read().split('\n')

problems = []

for i, line in enumerate(lines, 1):
    code = line.split('//')[0]
    stripped = code.strip()
    if not stripped:
        continue
    # Метод у выражения "Новый X(...)" — "Unknown operator".
    for m in re.finditer(r'Новый\s+[A-Za-zА-Яа-я0-9_]+\s*\([^()]*\)\s*\.', code):
        problems.append((i, 'метод у выражения Новый', m.group(0)))
    # Обращение к свойству у Новый Файл("...").Свойство тоже запрещено.
    for m in re.finditer(r'Новый\s+[A-Za-zА-Яа-я0-9_]+\s*\([^()]*\)\s*\.\s*[A-Za-zА-Яа-я_]', code):
        problems.append((i, 'обращение через точку к Новый', m.group(0)))
    # Непарные кавычки.
    if code.count('"') % 2 != 0:
        problems.append((i, 'непарные кавычки', stripped[:80]))
    # КонецПроцедуры/КонецФункции на одной строке с другим кодом.
    if re.search(r'(КонецПроцедуры|КонецФункции)\s*\S', code):
        problems.append((i, 'код после КонецПроцедуры', stripped[:80]))

# Баланс блоков.
opens = sum(1 for l in lines if re.match(r'\s*(Процедура|Функция)\s', l))
closes = sum(1 for l in lines if re.match(r'\s*(КонецПроцедуры|КонецФункции)\s*$', l))
print('объявлений процедур/функций:', opens, 'закрытий:', closes,
      'ОК' if opens == closes else 'РАСХОЖДЕНИЕ')

# Баланс Попытка/КонецПопытки и Если/КонецЕсли.
def count(pattern):
    return sum(len(re.findall(pattern, l.split('//')[0])) for l in lines)

pairs = [
    (r'\bПопытка\b', r'\bКонецПопытки\b'),
    (r'\bДля\s+каждого\b|\bДля\b', r'\bКонецЦикла\b'),
    (r'#Область', r'#КонецОбласти'),
]
for a, b in pairs:
    print(f'{a} = {count(a)}, {b} = {count(b)}')

if problems:
    print('ПРОБЛЕМЫ:')
    for i, kind, text in problems:
        print(f'  {i}: {kind}: {text}')
else:
    print('проблем не найдено')
