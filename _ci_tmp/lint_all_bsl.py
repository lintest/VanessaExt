import glob
import re

files = glob.glob('Example/Forms/**/*.bsl', recursive=True) + \
        glob.glob('Config/**/*.bsl', recursive=True)

found = False
for p in files:
    try:
        text = open(p, encoding='utf-8-sig').read()
    except UnicodeDecodeError:
        print('не UTF-8:', p)
        continue
    for i, line in enumerate(text.split('\n'), 1):
        code = line.split('//')[0]
        if re.search(r'Новый\s+[A-Za-zА-Яа-я0-9_]+\s*\([^()]*\)\s*\.\s*[A-Za-zА-Яа-я_]', code):
            print(f'{p}:{i}: {code.strip()}')
            found = True
if not found:
    print('конструкций "Новый X().Y" не найдено')
