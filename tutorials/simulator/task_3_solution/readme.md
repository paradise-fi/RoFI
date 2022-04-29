# Solution to task 3

*Assuming cwd is the RoFI.*

*Configuration hasn't changed from previous task.*

 1. Compile
    ```sh
    source setup.sh Debug
    rcfg desktop && rmake desktop
    ```

 2. Run the simulator
    ```sh
    source setup.sh Debug
    rofi-simplesim tutorials/simulator/task_3_solution/configuration.json
    ```

 3. Run the module code
    ```sh
    source setup.sh Debug
    tutorial-simulation_3_solution
    ```
