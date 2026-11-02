p = 'Example/Forms/AutotestRun/Ext/Form/Module.bsl'
lines = open(p, encoding='utf-8-sig').read().split('\n')
for i, l in enumerate(lines, 1):
    s = l.strip()
    if (s.startswith('Процедура ') or s.startswith('Функция ')
            or s.startswith('&') or s.startswith('#Область') or s.startswith('#КонецОбласти')
            or s.startswith('Перем ')):
        print(i, s)
