# Solution to task 6

*Assuming cwd is the RoFI.*

 1. Compile
    ```sh
    source setup.sh Debug
    rcfg desktop && rmake desktop
    ```

 2. Run the simulator with packet filter
    ```sh
    source setup.sh Debug
    rofi-simplesim tutorials/simulator/task_6_solution/configuration.json -p tutorials/simulator/task_6_solution/discard_half.py
    ```

 3. Run the module code **two times** in different terminals
    ```sh
    source setup.sh Debug
    tutorial-simulation_6_solution
    ```
