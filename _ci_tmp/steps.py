import json
import sys
import urllib.request

run_id = sys.argv[1]
want = sys.argv[2]

def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "ci"})
    return json.load(urllib.request.urlopen(req, timeout=60))

jobs = get(f"https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run_id}/jobs?per_page=20")
for j in jobs["jobs"]:
    if j["name"] != want:
        continue
    print(j["name"], j["status"], j["conclusion"])
    for s in j["steps"]:
        print(f'  {s["number"]:2} {s["name"]:45} {s["conclusion"]}')
