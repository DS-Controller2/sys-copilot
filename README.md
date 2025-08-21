# SystemMonitor

SystemMonitor is a C++/Qt-based system monitoring application designed for Linux. It provides a graphical user interface (GUI) to display real-time system resource usage, including CPU, memory, disk, and network statistics. Additionally, it features a robust process manager that allows users to view, kill, stop, and resume running processes. The application also includes a "Copilot" tab, which appears to be a chat-based assistant.

## Key Technologies

*   **C++:** The primary programming language used for the application's core logic.
*   **Qt:** A comprehensive framework utilized for developing the graphical user interface (GUI).
*   **CMake:** The build system employed to manage the project's compilation process and dependencies.

## Architecture

The application's codebase is organized into three main logical components:

*   `src/core`: Contains the fundamental logic responsible for gathering system information. The `SystemMonitor` class within this directory is specifically designed to collect data related to CPU, memory, disk, network, and active processes.
*   `src/ui`: Houses all the user interface code. The `MainWindow` class is central to this component, setting up the main application window, various tabs, and all the widgets necessary for displaying system information.
*   `src/common`: Stores common data structures, such as `SystemData`, which are shared and utilized across different parts of the application.

## Building and Running

To build and run the SystemMonitor application, ensure you have Qt and CMake installed on your Linux system. Follow these steps:

1.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

2.  **Run CMake to configure the project:**
    ```bash
    cmake ..
    ```

3.  **Compile the project:**
    ```bash
    make
    ```

4.  **Run the application:**
    ```bash
    ./SystemMonitor
    ```

## Development Conventions

*   **Coding Style:** The project adheres to standard Qt coding conventions, which include the use of `camelCase` for function names and variables.
*   **File Structure:** The project's files are logically separated into `src/core`, `src/ui`, and `src/common` directories to distinguish between core logic, user interface components, and shared data structures.
*   **Dependencies:** The application relies on the Qt Widgets and Qt Network modules for its functionality.
