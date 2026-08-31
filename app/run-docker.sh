#!/bin/bash
docker run --rm \
-p 8080:80 \
-v ./server-data:/server-data \
-v "$XDG_RUNTIME_DIR/pulse/native:/tmp/pulse/native" \
-e PULSE_SERVER=unix:/tmp/pulse/native \
racit-bell
