using System.Diagnostics;
using Microsoft.AspNetCore.Mvc;
using IrrigationSystemInterface.Models;
using IrrigationSystemInterface.Service;

namespace IrrigationSystemInterface.Controllers;

public class HomeController : Controller
{
    
    private readonly SocketService _socketConnection;

    public HomeController(SocketService socketConnection)
    {
        _socketConnection = socketConnection;
    }

    public async Task<IActionResult> Index()
    {
        await _socketConnection.EnsureConnectedAsync();

        // await _socketConnection.SendMessageAsync("RECEIVEDDATA");
        var message = await _socketConnection.ReceiveMessageAsync();
        Console.WriteLine("-------------RECEIVED----------");
        Console.WriteLine(message);
        Console.WriteLine("-------------RECEIVED----------");

        if (message == null)
        {
            return View();
        }

        var dataParts = message.Split(' ');
        Console.WriteLine("-------------DATA PARTS----------");
        foreach (var data in dataParts)
        {
            Console.WriteLine("Data : {0}" , data);
            
        }
        Console.WriteLine("-------------DATA PARTS----------");
        Console.WriteLine(dataParts.Length);

        if (dataParts.Length >= 5)
        {
            ViewData["Mode"] = dataParts[1];
            ViewData["Humidity"] = dataParts[2];
            ViewData["PumpState"] = dataParts[3];
            ViewData["ThreshHold"] = dataParts[4];
            ViewData["Anulata"] = dataParts[5];
            ViewData["SETARE"] = dataParts[6];

            Console.WriteLine("AICI");
            return View();
        }


        if (message is "FORCESTOP")
        {
            ViewData["ForceStop"] = message;
        }else if (message is "SETAT")
        {
            ViewData["SETARE"] = message;
        }else if (message is "PROGRAMARE ANULATA")
        {
            ViewData["Anulata"] = message;
        }
        else if(message is "CON")
        {
            ViewData["CON"] = message;
        }
        return View();
    }
    
    [HttpPost]
    public async Task<IActionResult> SendCommand(string command)
    {
        if (string.IsNullOrEmpty(command))
        {
            return BadRequest("Command cannot be null or empty.");
        }

        try
        {
            // Send command to the ESP8266
            var messageSent = await _socketConnection.SendMessageAsync(command + "\n");
            Console.WriteLine(messageSent);
            // Optionally, receive a response and process it
            // var response = await _socketConnection.ReceiveMessageAsync();
            // if (!string.IsNullOrEmpty(response))
            // {
            //     Console.WriteLine("AICI");
            //     // Process the response and update ViewData as needed
            //     if (response.StartsWith("HUMIDITY"))
            //     {
            //         ViewData["Humidity"] = response;
            //     }
            //     else if (response is "AUTO" or "MANUAL")
            //     {
            //         ViewData["Mode"] = response;
            //     }
            //     else if (response is "FORCESTOP")
            //     {
            //         ViewData["ForceStop"] = response;
            //     }
            //     else if (response is "ON" or "OFF")
            //     {
            //         ViewData["PumpState"] = response;
            //     }
            //     else if (response is "SETAT")
            //     {
            //         ViewData["SETARE"] = response;
            //     }
            //     else if (response is "PROGRAMARE ANULATA")
            //     {
            //         ViewData["Anulata"] = response;
            //     }
            //     else if (response is "CON")
            //     {
            //         ViewData["CON"] = response;
            //     }
            // }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error while sending command: {ex.Message}");
        }

        // Redirect back to the Index action
        return RedirectToAction("Index");
    }


    public IActionResult Privacy()
    {
        return View();
    }

    [ResponseCache(Duration = 0, Location = ResponseCacheLocation.None, NoStore = true)]
    public IActionResult Error()
    {
        return View(new ErrorViewModel { RequestId = Activity.Current?.Id ?? HttpContext.TraceIdentifier });
    }
}