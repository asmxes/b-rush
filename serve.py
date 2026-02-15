import asyncio
import json
import random
from datetime import datetime
from websockets.asyncio.server import serve

rooms: dict[str, dict[str, dict]] = {}

COLORS = [
    [255, 77, 77, 255],
    [77, 166, 255, 255],
    [77, 255, 136, 255],
    [255, 200, 50, 255],
    [200, 120, 255, 255],
]


def log(msg: str):
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {msg}")


def pick_color(room_id: str) -> list[int]:
    used = [c["color"] for c in rooms.get(room_id, {}).values()]
    available = [c for c in COLORS if c not in used]
    return random.choice(available) if available else random.choice(COLORS)


async def broadcast(room_id: str, message: str, exclude_guid: str | None = None):
    if room_id not in rooms:
        return
    for guid, client in rooms[room_id].items():
        if guid != exclude_guid:
            try:
                await client["ws"].send(message)
            except Exception as e:
                log(f"Failed to send to {guid[:12]}: {e}")


async def handler(ws):
    room_id = None
    guid = None

    try:
        async for raw in ws:
            msg = json.loads(raw)

            if msg["type"] == "join":
                room_id = msg["room"]
                guid = msg["guid"]

                if room_id not in rooms:
                    rooms[room_id] = {}

                color = pick_color(room_id)
                rooms[room_id][guid] = {"ws": ws, "color": color}

                peer_count = len(rooms[room_id])
                log(f"JOIN: {guid[:12]}... -> room '{room_id}' ({peer_count} peers)")

                await ws.send(json.dumps({
                    "type": "joined",
                    "guid": guid,
                    "color": color
                }))

                await broadcast(room_id, json.dumps({
                    "type": "peer_joined",
                    "guid": guid,
                    "color": color
                }), exclude_guid=guid)

                for peer_guid, peer in rooms[room_id].items():
                    if peer_guid != guid:
                        await ws.send(json.dumps({
                            "type": "peer_joined",
                            "guid": peer_guid,
                            "color": peer["color"]
                        }))

            # elif msg["type"] == "draw":
            #     await broadcast(room_id, json.dumps({
            #         "type": "draw",
            #         "guid": guid,
            #         "points": msg["points"]
            #     }), exclude_guid=guid)

            elif msg["type"] == "start_draw":
                await broadcast(room_id, json.dumps({
                    "type": "start_draw",
                    "guid": guid,
                    "x": msg["x"],
                    "y": msg["y"]
                }), exclude_guid=guid)

            elif msg["type"] == "add_point":
                await broadcast(room_id, json.dumps({
                    "type": "add_point",
                    "guid": guid,
                    "x": msg["x"],
                    "y": msg["y"]
                }), exclude_guid=guid)

            elif msg["type"] == "end_draw":
                await broadcast(room_id, json.dumps({
                    "type": "end_draw",
                    "guid": guid
                }), exclude_guid=guid)

            elif msg["type"] == "erase":
                log(f"ERASE: {guid[:12]}... in room '{room_id}'")
                await broadcast(room_id, json.dumps({
                    "type": "erase",
                    "guid": guid
                }), exclude_guid=guid)

    except Exception as e:
        log(f"ERROR: {e} (guid={guid[:12] if guid else 'unknown'}..., room={room_id})")
    finally:
        if room_id and guid and room_id in rooms:
            rooms[room_id].pop(guid, None)
            peer_count = len(rooms.get(room_id, {}))
            log(f"DISCONNECT: {guid[:12]}... from room '{room_id}' ({peer_count} peers remaining)")
            await broadcast(room_id, json.dumps({
                "type": "peer_left",
                "guid": guid
            }))
            if not rooms[room_id]:
                del rooms[room_id]
                log(f"ROOM CLOSED: '{room_id}'")


async def main():
    async with serve(handler, "0.0.0.0", 8765):
        log("Server running on ws://0.0.0.0:8765")
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
