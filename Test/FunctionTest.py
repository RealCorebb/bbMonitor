import asyncio
import json
import websockets

async def send_data():
    uri = "ws://192.168.253.203"  # Replace with the actual IP and port
    data_on = {"data": [1, 1, 1, 1, 1, 1, 1, 1]}
    data_half = {"data": [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5]}
    data_off = {"data":[0, 0, 0, 0, 0, 0, 0, 0]}

    try:
        async with websockets.connect(uri) as websocket:
            while True:
                print("send")
                await websocket.send(json.dumps(data_half))
                await asyncio.sleep(1)  # Delay for 1 second
                await websocket.send(json.dumps(data_on))
                await asyncio.sleep(1)  # Delay for 1 second
                await websocket.send(json.dumps(data_half))
                await asyncio.sleep(1)  # Delay for 1 second
                await websocket.send(json.dumps(data_off))
                await asyncio.sleep(1)  # Delay for 1 second
    except websockets.ConnectionClosedError as e:
        print(f"Connection closed: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(send_data())
