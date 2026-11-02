import json
import sys
import urllib.request

sha = sys.argv[1] if len(sys.argv) > 1 else '974efaf'

def get(url):
    req = urllib.request.Request(url, headers={
        "User-Agent": "ci", "Accept": "application/vnd.github+json"})
    return json.load(urllib.request.urlopen(req, timeout=60))

try:
    data = get(f"https://api.github.com/repos/lintest/VanessaExt/commits/{sha}/check-runs")
except Exception as e:
    print('check-runs:', type(e).__name__, e)
    data = {"check_runs": []}

for cr in data.get("check_runs", []):
    print(cr["name"], cr["status"], cr["conclusion"])
    try:
        ann = get(f'https://api.github.com/repos/lintest/VanessaExt/check-runs/{cr["id"]}/annotations')
        for a in ann:
            print('   ', a["annotation_level"], '|', a["message"][:400])
    except Exception as e:
        print('    annotations:', type(e).__name__, e)
