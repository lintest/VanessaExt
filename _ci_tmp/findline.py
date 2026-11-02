import sys

p = sys.argv[1]
need = [int(x) for x in sys.argv[2:]]
lines = open(p, encoding='utf-8-sig').read().split('\n')
for n in need:
    if 1 <= n <= len(lines):
        line = lines[n - 1]
        print(f'{n}: {line}')
        print(f'    символ {len(line)} : {line[38:46]!r}')
