# Entities

## AudioMeta

```json
{
    "name": "example.mp3",
    "duration_ms": 1234
}
```

- name: string, name of the audio file
- duration_ms: integer, duration of the audio in milliseconds

## QueuedPlayback

```json
{
    "id": 123,
    "audio_name": "example.mp3",
    "priority": 10,
    "enqueued_at": "2026-08-24T12:34:56Z"
}
```

- id: integer, ID of the object
- audio_name: string, name of the audio file
- priority: integer, priority of playback in queue
- enqueued_at: string, UTC time in ISO 8601

## DeferredPlayback

```json
{
    "id": 123,
    "audio_name": "example.mp3",
    "priority": 10,
    "play_time": "2026-08-24T12:34:56Z"
}
```

- id: integer, ID of the object
- audio_name: string, name of the audio file
- priority: integer, priority of playback in queue
- play_time: string, UTC time in ISO 8601

## ScheduleEntry

```json
{
    "audio_name": "example.mp3",
    "priority": 10,
    "time": "12:34:56"
}
```

- audio_name: string, name of the audio file
- priority: integer, priority of playback in queue
- time: string, time in format HH:MM:SS

## Schedule

```json
{
    "name": "example",
    "enabled": true,
    "entries": [
        {
            "audio_name": "example.mp3",
            "priority": 10,
            "time": "12:00:00"
        },
        {
            "audio_name": "audio.wav",
            "priority": 15,
            "time": "15:30:00"
        }
    ]
}
```

- name: string, unique name of the schedule
- enabled: boolean, determines whether schedule is enabled
- entries: array of ScheduleEntry

# General

If endpoint requires a JSON body, and there is no specified response for invalid or empty body, it will be a 400 json/application with body:
```json
{
    "error": "INVALID_BODY",
    "message": "Invalud request body."
}
```

Authorization needed for every request. Google ID token in HTTP header (Authorization: Bearer <token>) used for this purpose. If no token given, or person is unauthorized response will be 401 with empty body.

# /Sounds

## Get
    
Returns array of AudioMeta for all avaliable audio files.

Example response body:

```json
[
    {"name": "some_audio.mp3", "duration_ms": 1234},
    {"name": "another_audio.wav", "duration_ms": 5678}
]
```

## Post

Upload a new audio file.

- Request content type: multipart/form-data
- Required form field:
  - file: uploaded file

Server uses uploaded filename as stored audio name (in AudioMeta)

Responses:

- 200 with empty body
- 400 text/plain if form field is missing

# /Sounds/\<name\>

## Get

Download raw bytes of audio file.

Responses:

- Success:
    - 200 application/octet-stream
    - Body: raw bytes
- Not found:
    - 404 text/plain
    - Body: Audio not found

## Delete

Deletes an audio file from server.

Responses:

- Success:
    - 200 with empty body
- Not found:
    - 404 text/plain
    - Body: Audio not found

# /volume

## Get

Returns current master volume of playback as float.

Example response:

```json
{
    "volume": 1.0
}
```

## Post

Set master volume.

Expected body:

```json
{
    "volume": 0.5
}
```

Responses:

- Success:
    - 200 with empty body
- Invalid body
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_BODY",
            "message": "Invalud request body."
        }
        ```

# /queue

## Get

Returns all entries in queue as array of QueuedPlayback

## Post

Enqueues playback.

Expected body: QueuedPlayback without id and enqueued_at fields. Unnecessary fields will be ignored.

Responses:

- Success
    - 200 with empty body
- Invalid body
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_BODY",
            "message": "Invalud request body."
        }
        ```
- Aduio does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_AUDIO_NAME",
            "message": "Aduio with name '<name>' does not exist."
        }
        ```

# /queue/skip

## Post

Skips current playback.

Responses:

- Success
    - 200 with empty body
- Nothing playing
    - 400 application/json
    - Body:
        ```json
        {
            "error": "NOTHING_PLAYING",
            "message": "Nothing is playing right now."
        }
        ```

# /queue/\<id\>

## Delete

Removes entry from the queue.

Responses:
- Success
    - 200 with empty body
- Entry does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_ID",
            "message": "Queued playback with id '<id>' does not exist."
        }
        ```

# /deferred

## Get

Returns all deferred playbacks as array of DeferredPlayback

## Post

Adds new deferred playback.

Expected body: DeferredPlayback without id field. Unnecessary fields will be ignored.

Responses:
- Success
    - 200 with empty body
- Audio does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_AUDIO_NAME",
            "message": "Audio with name '<name>' does not exist."
        }
        ```

# /deferred/\<id\>

## Delete

Removes deferred playback by ID.

Responses:
- Success
    - 200 with empty body
- Deferred playback does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_ID",
            "message": "Deferred playback with id '<id>' does not exist."
        }
        ```

# /schedule

## Get

Returns all schedules as array of Schedule

## Post

Add new schedule.

Expected body: Schedule. Unnecessary fields will be ignored.

Responses:

- Success:
    - 200 with empty body
- Schedule name is already taken
    - 400 application/json
    - Body:
        ```json
        {
            "error": "SCHEDULE_NAME_ALREADY_TAKEN",
            "message": "Schedule with name '<name>' already exists."
        }
        ```
- Schedule is empty
    - 400 application/json
    - Body:
        ```json
        {
            "error": "SCHEDULE_EMPTY",
            "message": "Schedule has no entries."
        }
- Audio used in schedule does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_AUDIO_NAME",
            "message": "Audio with name '<name>' does not exist."
        }

## Put

Updates schedule with same name.

Expected body: Schedule. Unnecessary fields will be ignored.

Responses:

- Success:
    - 200 with empty body
- Schedule does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_SCHEDULE_NAME",
            "message": "Schedule with name '<name>' does not exist."
        }
        ```
- Schedule is empty
    - 400 application/json
    - Body:
        ```json
        {
            "error": "SCHEDULE_EMPTY",
            "message": "Schedule has no entries."
        }
- Audio used in schedule does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_AUDIO_NAME",
            "message": "Audio with name '<name>' does not exist."
        }

# /schedule/\<name\>

## Delete

Removes schedule.

Responses:

- Success:
    - 200 with empty body
- Schedule does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_SCHEDULE_NAME",
            "message": "Schedule with name '<name>' does not exist."
        }
        ```

# /schedule\<name\>/state

## Post

Sets schedule state (enabled/disabled).

Expected body:
```json
{
    "state": false
}
```

Responses:

- Success:
    - 200 with empty body
- Schedule does not exist
    - 400 application/json
    - Body:
        ```json
        {
            "error": "INVALID_SCHEDULE_NAME",
            "message": "Schedule with name '<name>' does not exist."
        }
        ```
