import subprocess

for f in ['Example/Forms/AutotestRun/Ext/Form/Module.bsl', '.github/workflows/build.yml']:
    head = subprocess.run(['git', 'show', f'HEAD:{f}'], capture_output=True).stdout
    d = open(f, 'rb').read()
    print(f)
    print('  HEAD   CRLF=%d LF=%d' % (head.count(b'\r\n'), head.count(b'\n')))
    print('  WORK   CRLF=%d LF=%d' % (d.count(b'\r\n'), d.count(b'\n')))
