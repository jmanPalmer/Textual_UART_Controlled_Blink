from textual.app import App
from textual.containers import HorizontalGroup,VerticalGroup
from textual.widgets import Header, Button, Label
import serial
from build import data_pb2

#open up so we can write to MCU, COM7 is what came up in device manager and baud rate of 9600 to match what we set in the C++ code
ser = serial.Serial("COM7", 9600)


#container for pannel that will hold the toggle on and off button
class ButtonPannel(VerticalGroup):


    def compose(self):
        yield HorizontalGroup(Button("Turn On", id = "on", variant="success"),  Button("Turn Off", id = "off", variant="error"))
        #yield Button("Turn On", id = "on", variant="success")
        #yield Button("Turn Off", id = "off", variant="error")
        yield Label("Bytes Sent:", id="byteDisplay")
        yield Label("STATUS: OFF", id = "statusDisplay")

    def on_button_pressed(self, event: Button.Pressed) -> None:

        #create object of BlinkData class created by protobuff
        msg = data_pb2.BlinkData()

        #assign proper values to our protobuff message
        if event.button.id == "on": #send serial data to turn on LED
            msg.toggle = True
            print("on")
        else: #send serial data to turn off LED
            msg.toggle = False
            print("off")

        #turn into a byte array using built in protobuff function and send it to the MC via UART
        data = msg.SerializeToString()
        length = len(data)
        packet = bytes([length]) + data #add the length byte to the front so it fits our protocol
        ser.write(packet)

        #debugging to just visualize whats being communicated
        hex_str = " ".join(f"0x{b:02X}" for b in packet)
        self.query_one("#byteDisplay", Label).update(f"Bytes Sent: {hex_str} Length: {len(packet)} bytes")

        response = ser.read(1) #wait for a response from MCU,read the status byte sent from MCU, will be one byte
        #again since this is our protocol we know what the mcu is going to send, in this case byte value of 0 or 1
        if(response[0]==1):
            self.query_one("#statusDisplay", Label).update(f"STATUS: ON")
        else: 
            self.query_one("#statusDisplay", Label).update(f"STATUS: OFF")

        

class ToggleApp(App):

    
    CSS = """
        Screen {
            align: center middle;
        }

        #buttonContainer {
            width: auto;
            height: auto;
            align: center middle;
        }

        ButtonPannel {
            width: auto;
        }

        #byteDisplay, #statusDisplay {
            width: auto;
        }
    """


    def compose(self):

        yield Header()
        yield ButtonPannel(id="buttonContainer")



if __name__ == "__main__":



    app = ToggleApp()
    app.run()

