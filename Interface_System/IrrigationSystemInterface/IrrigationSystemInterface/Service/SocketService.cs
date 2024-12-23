namespace IrrigationSystemInterface.Service;

public class SocketService
{
    private readonly SocketConnection _socketConnection;
    private bool _isConnected;

    public SocketService()
    {
        _socketConnection = new SocketConnection("192.168.1.211", 5055);
        // _socketConnection = new SocketConnection("192.168.1.143", 5055);
    }

    public async Task EnsureConnectedAsync()
    {
        if (!_isConnected)
        {
            int retries = 3;

            while (retries > 0)
            {
                try
                {
                    await _socketConnection.ConnectAsync();
                    _isConnected = true;
                    break;
                }
                catch
                {
                    retries--;
                    Console.WriteLine($"Retrying connection... ({3 - retries}/3)");
                    await Task.Delay(1000); // Wait 1 second before retrying
                }
            }

            if (!_isConnected)
            {
                throw new Exception("Failed to connect after 3 retries.");
            }
        }
    }


    public async Task<bool> SendMessageAsync(string message)
    {
        if (!_isConnected)
        {
            await EnsureConnectedAsync();
        }

        try
        {
            await _socketConnection.SendMessageAsync(message);
            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error sending message: {ex.Message}");
            _isConnected = false;
            return false;
        }
    }

    public async Task<string?> ReceiveMessageAsync()
    {
        if (!_isConnected)
        {
            await EnsureConnectedAsync();
        }

        try
        {
            return await _socketConnection.ReceiveMessageAsync();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error receiving message: {ex.Message}");
            _isConnected = false;
            return null;
        }
    }
}
