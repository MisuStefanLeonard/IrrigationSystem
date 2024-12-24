# HOW TO RUN

#### - In the files provided , you will find a file named SocketService.cs . In the constructor you will need to instantiate the Socket class using the IP and the PORT  provided in the arduino code <a href="https://github.com/MisuStefanLeonard/IrrigationSystem/tree/main/IrrigationSystem#setup" target="_blank">(See ESP code first)</a>
 . After you did that , run the code using and IDE (Visual Studio / Rider , etc ) and it will succesfully connect to the ESP and transmit data
#### - No auto refresh has been set on the page . Manual refresh is needed to update the data!


### The relevant part of the code where you connect to the ESP

```csharp
 public SocketService()
{
    _socketConnection = new SocketConnection("/*The ip address of the ESP*/", /*If you decide to change port , change in Arduino code as well*/5055);
}
```
