#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cmp firmware/common/HttpRequestParser.h firmware/ESP32_1/src/HttpRequestParser.h
cmp firmware/common/HttpRequestParser.h firmware/ESP32_2/src/HttpRequestParser.h
grep -Fq 'HWC::Http::parseRequest' firmware/common/EthernetStatus.h
cmp firmware/common/EthernetStatus.h firmware/ESP32_1/src/EthernetStatus.h
cmp firmware/common/EthernetStatus.h firmware/ESP32_2/src/EthernetStatus.h
grep -Fq 'Multipart' docs/HTTP_REQUEST_PARSER_STAGE_REV89.md
grep -Fq 'není zapojen' MASTER_HOME_WATER_CONTROLLER_3.12_REV89.md
echo 'PASS: HTTP parser is synchronized, documented and used by the shared Ethernet status server'
