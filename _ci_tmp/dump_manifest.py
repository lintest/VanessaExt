import re, sys

src = open('.github/workflows/build.yml', encoding='utf-8').read().split('\n')

start = None
for i, l in enumerate(src):
    if 'cat > manifest.xml' in l:
        start = i
        break
print('start line', start + 1)

out = []
for l in src[start:]:
    out.append(l)

# strip common indentation of the yaml block (10 spaces)
body = [l[10:] if l.startswith(' ' * 10) else l for l in out]
end = None
for i, l in enumerate(body):
    if l.strip() == 'EOF':
        end = i
        break
manifest = '\n'.join(body[1:end]) + '\n'
print('--- manifest bytes ---')
print(repr(manifest[:200]))
print('--- rendered ---')
print(manifest)
open('_ci_tmp/manifest_preview.xml', 'w', encoding='utf-8').write(manifest)

import xml.dom.minidom
try:
    d = xml.dom.minidom.parseString(manifest.encode('utf-8'))
    print('XML OK, root:', d.documentElement.tagName,
          'components:', len(d.getElementsByTagName('component')))
except Exception as e:
    print('XML BROKEN:', e)
