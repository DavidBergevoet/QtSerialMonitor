#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QSerialPort>
#include <QThread>
#include <iostream>
#include <sys/ioctl.h>

#include "QRegularExpression"

QSerialPort port;

const char* READ_COMMAND = "a";

void HandleSerialRead()
{
  std::cout << port.readAll().replace("\n", "<NL>").replace("\r", "<CR>").toStdString() << std::endl;
}

void SendCommand()
{
  std::string command;
  std::cin >> command;
  if (command != READ_COMMAND)
  {
    port.write((command + "\r\n").c_str());
  }
  port.flush();
}

QVector<QString> previousCommands;
int currentIndex = 0;
QString GetNextPreviousCommand(QString current)
{
  if (currentIndex < previousCommands.size())
  {
    int oldIndex = currentIndex++;
    const QString returnValue = previousCommands[oldIndex];
    return returnValue;
  }
  return current;
}
QString GetPreviousPreviousCommand()
{
  if (currentIndex > 0)
  {
    currentIndex--;
    const QString returnValue = previousCommands[currentIndex];
    return returnValue;
  }
  return "";
}
char getch()
{
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror("tcsetattr ~ICANON");
  return (buf);
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  std::string portName;
  uint32_t baud = 0;
  if (argc != 3)
  {
    std::cout << "Port: ";
    std::cin >> portName;

    std::cout << "Baudrate: ";
    std::cin >> baud;
  }
  else
  {
    portName = argv[1];
    baud = std::stoull(argv[2]);
  }
  port.setPortName(QString::fromStdString(portName));
  port.setBaudRate(baud);

  if (port.open(QIODevice::ReadWrite))
  {
    std::cout << "Opened: " << argv[1] << std::endl;
  }
  else
  {
    std::cout << "Cant open port" << std::endl;
    exit(1);
  }

  struct winsize windowSize;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize);

  QTextStream consoleOutput(stdout);

  while (true)
  {
    QString userInput;
    while (true)
    {
      consoleOutput << portName.c_str() << "# " << userInput;
      consoleOutput.flush();
      char c = getch();
      switch (c)
      {
      case 27: // Handle Escape character
      {
        if (getch() == 91) // Handle arrow keys
        {
          switch (getch())
          {
          case 'A': // Arrow up
          {
            userInput = GetNextPreviousCommand(userInput);
            break;
          }
          case 'B': // Arrow down
          {
            userInput = GetPreviousPreviousCommand();
            break;
          }
          default:
          {
            consoleOutput << "Unhandled escape character!\n";
            consoleOutput.flush();
          }
          }
        }
        break;
      }
      case 127: // Backspace
      {
        userInput = userInput.remove(userInput.size() - 1, 1);
        break;
      }
      default:
      {
        if (std::isprint(c) || c == 10)
          userInput += c;
        else
          consoleOutput << "\nCharacter is not isprint " << (int)c << "\n";
      }
      }
      if (userInput.contains('\n'))
        break;
      consoleOutput << "\r";
      for (int i = 0; i < windowSize.ws_col; ++i)
        consoleOutput << " ";
      consoleOutput << "\r";

      consoleOutput.flush();
    }
    userInput.remove("\n");
    currentIndex = 0;
    if (!previousCommands.contains(userInput))
      previousCommands.prepend(userInput);

    consoleOutput << "\n";
    consoleOutput.flush();

    if (userInput.toLower() == "quit")
    {
      break;
    }

    QByteArray sendData = userInput.toUtf8() + "\r";
    port.write(sendData);
    port.flush();

    app.processEvents();
    QThread::currentThread()->sleep(1);
    app.processEvents();

    QByteArray receivedData = port.readAll().replace("\n", "<NL>").replace("\r", "<CR>");
    consoleOutput << "Received: '" << receivedData << "'" << Qt::endl;
  }
  port.close();
  return 0;
}
