import sys

for f in sys.argv[1:]:
    d = open(f, 'rb').read()
    if not d.count(b'\r\n'):
        print('ok, no changes:', f)
        continue
    n = d.replace(b'\r\n', b'\n')
    open(f, 'wb').write(n)
    print('normalized:', f, 'removed', d.count(b'\r\n'), 'CRLF')
