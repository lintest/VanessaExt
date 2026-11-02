import json
import sys
import urllib.request

run_id = sys.argv[1] if len(sys.argv) > 1 else None

def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "ci"})
    return json.load(urllib.request.urlopen(req, timeout=60))

if run_id is None:
    data = get("https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=1")
    run_id = data["workflow_runs"][0]["id"]
    print("run", data["workflow_runs"][0]["run_number"], run_id)

jobs = get(f"https://api.github.com/repos/lintest/VanessaExt/actions/runs/{run_id}/jobs?per_page=20")
for j in jobs["jobs"]:
    print(f'{j["name"]:18} {j["status"]:12} {j["conclusion"]}')
    if j["conclusion"] not in (None, "success"):
        for s in j["steps"]:
            mark = "  " if s["conclusion"] == "success" else "!!"
            print(f'   {mark} {s["number"]:2} {s["name"]:45} {s["conclusion"]}')
