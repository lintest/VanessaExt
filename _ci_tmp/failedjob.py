import json
import sys
import urllib.request

sha = sys.argv[1]

def get(url):
    req = urllib.request.Request(url, headers={
        "User-Agent": "ci", "Accept": "application/vnd.github+json"})
    return json.load(urllib.request.urlopen(req, timeout=60))

data = get("https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=5")
run = None
for r in data["workflow_runs"]:
    if r["head_sha"].startswith(sha):
        run = r
        break
jobs = get(f'https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run["id"]}/jobs?per_page=20')
for j in jobs["jobs"]:
    if j["conclusion"] not in ("failure",):
        continue
    print(j["name"], j["conclusion"])
    for s in j["steps"]:
        mark = "!!" if s["conclusion"] == "failure" else "  "
        print(f'  {mark} {s["number"]:2} {s["name"]:45} {s["conclusion"]}')
    ann = get(f'https://api.github.com/repos/lintest/VanessaExt/check-runs/{j["id"]}/annotations')
    for a in ann:
        if "Node.js 20" in a["message"] or "ubuntu-latest label" in a["message"]:
            continue
        print('  >>', a["annotation_level"], '|', a["message"][:2500])
