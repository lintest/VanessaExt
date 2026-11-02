import json
import sys
import time
import urllib.error
import urllib.request

sha = sys.argv[1]

def get(url, tries=3):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers={
                "User-Agent": "ci", "Accept": "application/vnd.github+json"})
            return json.load(urllib.request.urlopen(req, timeout=60))
        except urllib.error.HTTPError as e:
            if e.code in (403, 429) and i + 1 < tries:
                print("rate limit, ждём 120 с", flush=True)
                time.sleep(120)
                continue
            raise

runs = get("https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=5")
run = None
for r in runs["workflow_runs"]:
    if r["head_sha"].startswith(sha):
        run = r
        break
if run is None:
    print("прогон не найден:", sha)
    sys.exit(0)
print("run", run["run_number"], run["status"], run["conclusion"], run["html_url"])

jobs = get(f'https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run["id"]}/jobs?per_page=20')
for j in jobs["jobs"]:
    print(f'  {j["name"]:16} {j["status"]:12} {j["conclusion"]}')
    if j["name"] != "linux-test":
        continue
    for s in j["steps"]:
        print(f'     {s["number"]:2} {s["name"]:42} {s["conclusion"]}')
    checks = get(f'https://api.github.com/repos/lintest/VanessaExt/check-runs/{j["id"]}/annotations')
    for a in checks:
        print('  >>', a["annotation_level"], '|', a["message"][:3000])
