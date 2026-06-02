import usocket as socket
import ustruct as struct

class MQTTClient:

    def __init__(self, client_id, server, port=1883):
        self.client_id = client_id
        self.server = server
        self.port = port
        self.cb = None

    def set_callback(self, f):
        self.cb = f

    def connect(self):
        self.sock = socket.socket()
        self.sock.connect(
            (
                self.server,
                self.port
            )
        )

        msg = bytearray(
            b"\x10\0\0\0\0\0"
        )

        msg.extend(
            b"\x04MQTT\x04\x02\0<"
        )

        msg.extend(
            struct.pack(
                "!H",
                len(self.client_id)
            )
        )

        msg.extend(
            self.client_id.encode()
        )

        msg[1] = len(msg)-2

        self.sock.send(msg)

        self.sock.recv(4)

    def subscribe(self, topic):

        pkt = bytearray(
            b"\x82\0\0\1"
        )

        pkt.extend(
            struct.pack(
                "!H",
                len(topic)
            )
        )

        pkt.extend(topic)

        pkt.append(0)

        pkt[1] = len(pkt)-2

        self.sock.send(pkt)

        self.sock.recv(5)

    def check_msg(self):

        self.sock.setblocking(False)

        try:

            op = self.sock.recv(1)

            if not op:
                return

            if op[0] == 0x30:

                self.sock.recv(1)

                topic_len = struct.unpack(
                    "!H",
                    self.sock.recv(2)
                )[0]

                topic = self.sock.recv(
                    topic_len
                )

                payload = self.sock.recv(
                    100
                )

                if self.cb:
                    self.cb(
                        topic,
                        payload
                    )

        except:
            pass