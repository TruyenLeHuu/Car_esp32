import string
import paho.mqtt.client as mqtt #import the client1
import time
import logging
from Script.Component.MQTTComp import MQTTComp
from enum import Enum

class PublishType(Enum):
    CONTROL = 1,

class MQTTClientController():
    def __init__(self, _mqttComp: MQTTComp, _clientName: string):
        self.mqttComp = _mqttComp
        self.client = mqtt.Client(_clientName)
        self.client.connect(self.mqttComp.brokerIP)
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_connect_fail = self.on_connect_fail
        self.client.on_message = self.on_message

    # The callback for when the client receives a CONNACK response from the server.
    def on_connect(self, client, rc):
        # logging.info("Connected with result code "+str(rc))
        print("Connected with result code "+str(rc))
        self.mqttComp.connectStatus = True
        # Subscribing in on_connect() means that if we lose the connection and
        # reconnect then subscriptions will be renewed.
        client.subscribe(self.mqttComp.commandTopic)
        if (self.mqttComp.isTimeStamp):
            client.subscribe(self.mqttComp.timestampTopic)

    def on_disconnect(self):
        self.mqttComp.connectStatus = False
    
    def on_connect_fail(self):
        self.mqttComp.connectStatus = False

    # The callback for when a PUBLISH message is received from the server.
    def on_message(self, client, userdata, msg):
        print(msg.topic+" "+str(msg.payload))
        msgContent = msg.payload.decode("utf-8")

    def publish_message(self, type : PublishType, message):
        topic = "NULL"
        if (type == PublishType.CONTROL):
            topic = self.mqttComp.controlTopic

        self.client.publish(topic, message)
