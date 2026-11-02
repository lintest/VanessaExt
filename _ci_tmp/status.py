import json
import sys
import urllib.request

sha = sys.argv[1] if len(sys.argv) > 1 else None

def get(url):
    req = urllib.request.Request(url, headers={
        "User-Agent": "ci", "Accept": "application/vnd.github+json"})
    return json.load(urllib.request.urlopen(req, timeout=60))

data = get("https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=5")
run = data["workflow_runs"][0]
if sha:
    for r in data["workflow_runs"]:
        if r["head_sha"].startswith(sha):
            run = r
            break
print("run", run["run_number"], run["head_sha"][:7], run["status"], run["conclusion"])

jobs = get(f'https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run["id"]}/jobs?per_page=20')
for j in jobs["jobs"]:
    print(f'  {j["name"]:16} {j["status"]:12} {j["conclusion"]}')

# Аннотации только для linux-test - это главный сигнал.
for j in jobs["jobs"]:
    if j["name"] != "linux-test" or j["conclusion"] is None:
        continue
    ann = get(f'https://api.github.com/repos/lintest/VanessaExt/check-runs/{j["id"]}/annotations')
    for a in ann:
        if "Node.js 20" in a["message"] or "ubuntu-latest label" in a["message"]:
            continue
        print('  >>', a["annotation_level"], '|', a["message"][:2000])
