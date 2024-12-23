using System.Net.Sockets;
using System.Net.WebSockets;
using System.Text;

namespace IrrigationSystemInterface;

public class SocketConnection
{
    private readonly Socket _socket;
    private readonly string _serverAddress;
    private readonly int _port;

    
    public SocketConnection(string serverAddress , int port)
    {
        _socket = new Socket(AddressFamily.InterNetwork , SocketType.Stream , ProtocolType.Tcp);
        _socket.SetSocketOption(SocketOptionLevel.Socket , SocketOptionName.KeepAlive , true);
        _serverAddress = serverAddress;
        _port = port;
    }
    
    private async Task ReconnectIfNeededAsync()
    {
        if (!_socket.Connected)
        {
            Console.WriteLine("Socket disconnected. Reconnecting...");
            await ConnectAsync();
        }
    }
    public async Task ConnectAsync()
    {
        try
        {
            await _socket.ConnectAsync(_serverAddress , _port);
            Console.WriteLine($"Connected to WebSocket server at {_serverAddress}:{_port}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error connecting to WebSocket server: {ex.Message}");
        }
    }

    public async Task<string> SendMessageAsync(string message)
    {
        try
        {
            if (!_socket.Connected)
            {
                await ReconnectIfNeededAsync();
            }
            if (_socket.Connected)
            {
                var messageBytes = Encoding.UTF8.GetBytes(message );
                await _socket.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
                Console.WriteLine($"--------SENT------");
                Console.WriteLine($"Message sent: {message}");
                Console.WriteLine($"--------SENT------");

                return message;
            }else
            {
                Console.WriteLine("WebSocket is not connected. Unable to send message.");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error sending message to ESP: {ex.Message}");
        }

        return "";
    }
    
    public async Task<string?> ReceiveMessageAsync()
    {
        try
        {
            if (_socket.Connected)
            {
                var buffer = new byte[1024];
                using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(10));
                var bytesRead = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None, cts.Token);
                if (bytesRead > 0)
                {
                    return Encoding.UTF8.GetString(buffer, 0, bytesRead);
                }
            }
            return null;
        }
        catch (OperationCanceledException)
        {
            // Timeout handling
            Console.WriteLine("Receive operation timed out.");
            return null;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error receiving message: {ex.Message}");
            return null;
        }
    }


}