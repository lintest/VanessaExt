import io
import json
import sys
import urllib.request
import zipfile

run_id = sys.argv[1]
want = sys.argv[2] if len(sys.argv) > 2 else None

def get(url, raw=False):
    req = urllib.request.Request(url, headers={"User-Agent": "ci"})
    r = urllib.request.urlopen(req, timeout=120)
    data = r.read()
    return data if raw else json.loads(data)

arts = get(f"https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run_id}/artifacts?per_page=50")
for a in arts["artifacts"]:
    print(a["id"], a["name"], a["size_in_bytes"], "expired" if a["expired"] else "ok")

target = None
for a in arts["artifacts"]:
    if want and a["name"] == want:
        target = a
if target is None:
    sys.exit(0)

try:
    blob = get(target["archive_download_url"], raw=True)
    z = zipfile.ZipFile(io.BytesIO(blob))
    print("файлы:", z.namelist())
    for name in z.namelist():
        if name.endswith(('trace.log', '.txt', 'success.txt')):
            print('=' * 20, name)
            print(z.read(name).decode('utf-8', 'replace')[:4000])
except Exception as e:
    print("ошибка скачивания:", type(e).__name__, e)
