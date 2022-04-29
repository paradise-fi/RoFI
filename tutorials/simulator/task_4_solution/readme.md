# Solution to task 4

*Assuming cwd is the RoFI.*

 1. Compile
    ```sh
    source setup.sh Debug
    rcfg desktop && rmake desktop
    ```

 2. Run the simulator
    ```sh
    source setup.sh Debug
    rofi-simplesim tutorials/simulator/task_4_solution/configuration.json
    ```

 3. Run the module code **three times** in different terminals
    ```sh
    source setup.sh Debug
    tutorial-simulation_4_solution
    ```
