import sys

try:
    import yaml
except ImportError:
    print('PyYAML не установлен')
    sys.exit(0)

for f in ['.github/workflows/build.yml']:
    with open(f, encoding='utf-8') as fh:
        doc = yaml.safe_load(fh)
    print(f, 'OK')
    jobs = doc['jobs']
    print('  jobs:', list(jobs))
    steps = jobs['linux-test']['steps']
    for s in steps:
        if s.get('name') == 'Выгрузка артефактов':
            print('  артефакты:', s['with']['path'].split('\n'))
    ms = steps[-(len(steps)):]
