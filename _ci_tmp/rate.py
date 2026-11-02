import json
import time
import urllib.request

req = urllib.request.Request("https://api.github.com/rate_limit",
                             headers={"User-Agent": "ci"})
data = json.load(urllib.request.urlopen(req, timeout=30))
core = data["resources"]["core"]
reset = time.strftime("%H:%M:%S", time.localtime(core["reset"]))
print("осталось", core["remaining"], "из", core["limit"], "сброс в", reset,
      "через", max(0, core["reset"] - int(time.time())), "с")
