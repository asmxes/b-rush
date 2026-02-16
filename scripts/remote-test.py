import asyncio, websockets, json


async def test():
    async with websockets.connect('wss://ws.b-rush.it') as ws:
        await ws.send(json.dumps({'type': 'join', 'room': 'test:Blue', 'guid': 'test123'}))
        print(await ws.recv())


if __name__ == "__main__":
    asyncio.run(test())
