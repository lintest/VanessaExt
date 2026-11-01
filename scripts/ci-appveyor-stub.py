#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Заглушка AppVeyor API для прогона автотестов VanessaExt в GitHub Actions.

Зачем это нужно. Внешняя обработка Example/Forms/AutotestRun при открытии читает
рядом с собой файл app_port.txt, поднимает HTTPСоединение на localhost с этим
портом и отправляет туда результаты тестов (PUT /api/tests) и сообщения
(POST /api/build/messages). Файл писал appveyor.ps1, а порт принадлежал API
AppVeyor. Без app_port.txt форма падает на чтении файла ещё до запуска тестов,
поэтому на раннере GitHub Actions поднимается этот приёмник, а его порт
записывается в app_port.txt. Код 1С при этом не меняется.

Результаты прогона складываются в tests-report.json (машинночитаемо) и
tests-report.log (глазами); оба файла перезаписываются после каждого запроса,
поэтому отчёт уцелеет, даже если процесс прибьют по таймауту.

Запуск:
    python scripts/ci-appveyor-stub.py
    python scripts/ci-appveyor-stub.py --output-dir .

Код возврата: 0 — сервер штатно остановлен.
"""

import argparse
import json
import os
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

TESTS = []
MESSAGES = []

OUTPUT_DIR = "."


def flush_report():
    """Переписать отчёты на диск. Вызывается после каждого запроса."""
    report = {"tests": TESTS, "messages": MESSAGES}
    path = os.path.join(OUTPUT_DIR, "tests-report.json")
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2)

    path = os.path.join(OUTPUT_DIR, "tests-report.log")
    with open(path, "w", encoding="utf-8") as handle:
        for test in TESTS:
            handle.write("%-8s %s (%s мс)\n" % (
                test.get("outcome", "?"),
                test.get("testName", "?"),
                test.get("durationMilliseconds", 0)))
            if test.get("ErrorMessage"):
                handle.write("         %s\n" % test["ErrorMessage"])
        for message in MESSAGES:
            handle.write("%-8s %s\n" % (
                message.get("category", "?"), message.get("message", "?")))
            if message.get("details"):
                handle.write("         %s\n" % message["details"])

        failed = [test for test in TESTS if test.get("outcome") != "Passed"]
        handle.write("\nтестов: %d, провалено: %d\n" % (len(TESTS), len(failed)))


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, format, *args):
        # Штатный лог http.server не нужен: пишем свой, в stdout шага.
        pass

    def read_payload(self):
        length = int(self.headers.get("Content-Length") or 0)
        if not length:
            return {}
        raw = self.rfile.read(length)
        try:
            return json.loads(raw.decode("utf-8"))
        except ValueError:
            return {"raw": raw.decode("utf-8", "replace")}

    def reply(self):
        payload = self.read_payload()
        path = self.path.split("?")[0]

        if path.endswith("/api/tests"):
            TESTS.append(payload)
            print("тест: %s -> %s" % (
                payload.get("testName"), payload.get("outcome")), flush=True)
        elif path.endswith("/api/build/messages"):
            MESSAGES.append(payload)
            print("сообщение: [%s] %s" % (
                payload.get("category"), payload.get("message")), flush=True)
        else:
            print("%s %s (без обработки)" % (self.command, path), flush=True)

        flush_report()

        body = b'{"status":"ok"}'
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    do_GET = reply
    do_POST = reply
    do_PUT = reply


def main():
    global OUTPUT_DIR

    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass

    parser = argparse.ArgumentParser(
        description="Заглушка AppVeyor API для автотестов VanessaExt")
    parser.add_argument("--output-dir", default=".",
                        help="куда писать app_port.txt и отчёты")
    args = parser.parse_args()
    OUTPUT_DIR = args.output_dir

    flush_report()

    # Порт 0 — свободный порт выбирает ядро. Порт нужен 1С, поэтому сразу
    # отдаём его через app_port.txt, как это делал appveyor.ps1.
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    port = server.server_address[1]
    with open(os.path.join(OUTPUT_DIR, "app_port.txt"), "w", encoding="utf-8") as handle:
        handle.write("%d\n" % port)
    print("приёмник слушает порт %d" % port, flush=True)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
