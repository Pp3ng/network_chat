#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <system_error>
#include <ctime>
#include <array>
#include <optional>
#include <format>
#include <asio.hpp>

class ChatClient
{
public:
    ChatClient(const std::string &server_ip, uint16_t port)
        : io_context_(), socket_(io_context_), running_(true)
    {
        try
        {
            asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(server_ip), port);
            socket_.connect(endpoint);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::format("Connection failed: {}", e.what()));
        }
    }

    ~ChatClient()
    {
        stop();
    }

    void start(const std::string &username, int channel)
    {
        try
        {
            send_message(username);
            send_message(std::to_string(channel));

            receive_thread_ = std::thread([this]
                                          { receive_messages(); });
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::format("Failed to start client: {}", e.what()));
        }
    }

    void stop()
    {
        running_ = false;
        if (socket_.is_open())
        {
            std::error_code ec;
            socket_.close(ec);
        }
        if (receive_thread_.joinable())
        {
            receive_thread_.join();
        }
    }

    void send_message(const std::string &message)
    {
        try
        {
            asio::write(socket_, asio::buffer(message + "\n"));
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::format("Failed to send message: {}", e.what()));
        }
    }

    static void print_help()
    {
        std::cout << "Available commands:\n"
                  << "/quit - Exit the chat\n"
                  << "/w <username> <message> - Send a private message\n"
                  << "/list - List all connected users\n"
                  << "/history - Request chat history\n"
                  << "/clear - Clear the screen\n"
                  << "/help - Show this help message\n";
    }

private:
    void receive_messages()
    {
        std::array<char, 1024> buffer;

        while (running_)
        {
            try
            {
                std::error_code ec;
                std::size_t length = socket_.read_some(asio::buffer(buffer), ec);

                if (ec)
                {
                    if (running_)
                    {
                        std::cout << "\rServer disconnected\n";
                    }
                    break;
                }

                if (length > 0)
                {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    auto *timeinfo = std::localtime(&time);

                    std::cout << std::format("\r[{:02d}:{:02d}:{:02d}] {}> ",
                                             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                                             std::string_view(buffer.data(), length));
                    std::cout.flush();
                }
            }
            catch (const std::exception &e)
            {
                if (running_)
                {
                    std::cerr << "\rError receiving message: " << e.what() << "\n";
                }
                break;
            }
        }
    }

    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
    std::atomic<bool> running_;
    std::thread receive_thread_;
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << std::format("Usage: {} <server_ip>\n", argv[0]);
        return 1;
    }

    try
    {
        ChatClient client(argv[1], 8042);

        std::string username;
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);

        std::string channel_str;
        std::cout << "Enter channel number: ";
        std::getline(std::cin, channel_str);
        int channel = std::stoi(channel_str);

        client.start(username, channel);
        ChatClient::print_help();

        std::string input;
        while (std::cout << "> " && std::getline(std::cin, input))
        {
            if (input == "/quit")
            {
                break;
            }
            else if (input == "/help")
            {
                ChatClient::print_help();
            }
            else if (input == "/clear")
            {
                std::cout << "\033[H\033[J";
            }
            else
            {
                client.send_message(input);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}