import glob

files = [
    'Example/Forms/AutotestRun/Ext/Form/Module.bsl',
    'scripts/ci-linux-test.sh',
    'scripts/ci-linux-build.sh',
    '.github/workflows/build.yml',
]
for f in files:
    d = open(f, 'rb').read()
    crlf = d.count(b'\r\n')
    lf = d.count(b'\n')
    print(f, 'CRLF=', crlf, 'LF=', lf, 'bare-LF=', lf - crlf, 'BOM=', d[:3] == b'\xef\xbb\xbf')
