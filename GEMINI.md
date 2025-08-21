# Project Overview

This project is a C++/Qt-based system monitoring application for Linux. It provides a graphical user interface to display real-time system resource usage, including CPU, memory, disk, and network. It also features a process manager that allows users to view, kill, stop, and resume processes. Additionally, it includes a "Copilot" tab that appears to be a chat-based assistant.

**Key Technologies:**

*   **C++:** The core language used for the application.
*   **Qt:** The framework used for the graphical user interface (GUI).
*   **CMake:** The build system used to manage the project's compilation and dependencies.

**Architecture:**

The application is structured into three main parts:

*   **`src/core`:** Contains the core logic for gathering system information. The `SystemMonitor` class is responsible for collecting data on CPU, memory, disk, network, and processes.
*   **`src/ui`:** Contains the user interface code. The `MainWindow` class sets up the main window, tabs, and all the widgets for displaying system information.
*   **`src/common`:** Contains common data structures, such as `SystemData`, used throughout the application.

# Building and Running

To build and run the project, you will need to have Qt and CMake installed.

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

# Development Conventions

*   **Coding Style:** The code follows the standard Qt coding conventions, including the use of camelCase for function names and variables.
*   **File Structure:** The project is organized into `src/core`, `src/ui`, and `src/common` directories to separate the core logic, UI, and common data structures.
*   **Dependencies:** The project depends on the Qt Widgets and Network modules.
