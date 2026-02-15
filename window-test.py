import asyncio
import json
import sys
import threading
import pygame

SERVER_URL = "ws://localhost:8765"
ROOM = sys.argv[1] if len(sys.argv) > 1 else "test:Blue"
GUID = "viewer-" + str(id(object()))

BG_COLOR = (20, 20, 30)
GRID_COLOR = (40, 40, 50)

# peer_guid -> { "color": (r,g,b,a), "segments": [[(x,y), ...], ...] }
peers: dict[str, dict] = {}
lock = threading.Lock()


async def ws_client():
    import websockets

    async with websockets.connect(SERVER_URL) as ws:
        await ws.send(json.dumps({
            "type": "join",
            "room": ROOM,
            "guid": GUID
        }))

        async for raw in ws:
            msg = json.loads(raw)

            with lock:
                if msg["type"] == "joined":
                    pass

                elif msg["type"] == "peer_joined":
                    guid = msg["guid"]
                    if guid not in peers:
                        peers[guid] = {
                            "color": tuple(msg["color"][:3]),
                            "segments": []
                        }

                elif msg["type"] == "draw":
                    guid = msg["guid"]
                    if guid not in peers:
                        peers[guid] = {
                            "color": (200, 200, 200),
                            "segments": []
                        }
                    points = [(p["x"], p["y"]) for p in msg["points"]]
                    peers[guid]["segments"].append(points)

                elif msg["type"] == "start_draw":
                    guid = msg["guid"]
                    if guid not in peers:
                        peers[guid] = {
                            "color": (200, 200, 200),
                            "segments": []
                        }
                    peers[guid]["segments"].append([(msg["x"], msg["y"])])

                elif msg["type"] == "add_point":
                    guid = msg["guid"]
                    if guid in peers and peers[guid]["segments"]:
                        peers[guid]["segments"][-1].append((msg["x"], msg["y"]))

                elif msg["type"] == "end_draw":
                    pass

                elif msg["type"] == "erase":
                    guid = msg["guid"]
                    if guid in peers:
                        peers[guid]["segments"].clear()

                elif msg["type"] == "peer_left":
                    guid = msg["guid"]
                    peers.pop(guid, None)


def normalized_to_screen(nx, ny, w, h):
    return (
        int(nx * w + w * 0.5),
        int(ny * h + h * 0.5)
    )


def main():
    pygame.init()
    screen = pygame.display.set_mode((800, 600), pygame.RESIZABLE)
    pygame.display.set_caption(f"Viewer - {ROOM}")
    clock = pygame.time.Clock()

    def run_ws():
        asyncio.run(ws_client())

    t = threading.Thread(target=run_ws, daemon=True)
    t.start()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.VIDEORESIZE:
                screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE)

        w, h = screen.get_size()
        screen.fill(BG_COLOR)

        for x in range(0, w, 50):
            pygame.draw.line(screen, GRID_COLOR, (x, 0), (x, h))
        for y in range(0, h, 50):
            pygame.draw.line(screen, GRID_COLOR, (0, y), (w, y))

        cx, cy = w // 2, h // 2
        pygame.draw.line(screen, (60, 60, 70), (cx - 20, cy), (cx + 20, cy))
        pygame.draw.line(screen, (60, 60, 70), (cx, cy - 20), (cx, cy + 20))

        with lock:
            for guid, peer in peers.items():
                color = peer["color"]
                for segment in peer["segments"]:
                    if len(segment) < 2:
                        continue
                    screen_points = [
                        normalized_to_screen(p[0], p[1], w, h) for p in segment
                    ]
                    pygame.draw.lines(screen, color, False, screen_points, 3)
                    pygame.draw.circle(screen, color, screen_points[0], 4)
                    pygame.draw.circle(screen, color, screen_points[-1], 4)

        with lock:
            y_off = 10
            for guid, peer in peers.items():
                label = pygame.font.SysFont("consolas", 14).render(
                    f"{guid[:12]}...", True, peer["color"]
                )
                screen.blit(label, (10, y_off))
                y_off += 20

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()


if __name__ == "__main__":
    main()
