# Simulator Tutorial

```{tip}
It is recommended that after each task you try to compile and run your code.
```


## Task 0: Setup

### Compile and run SimpleSim
See [RoFI Compilation](https://paradise-fi.github.io/RoFI/intro/compiling).
For this tutorial you will need the
[desktop suite](https://paradise-fi.github.io/RoFI/intro/suites/desktop).

After installing all the prerequisities, run Simplesim using the
[Quick start SimpleSim example](https://paradise-fi.github.io/RoFI/intro/compiling#simplesim).

### Create a folder for your solution

Create a directory for your solution in `tutorials/simulator/`
(with any name that you like).

```{note}
You may be able to create the directory anywhere, but right now the easiest
way to include everything is to work inside the `RoFI` repository.
```


## Task 1: Create a Configuration

Create a file `configuration.json` that will contain a configuration with:
 - id 12: Universal module
 - id 42: Pad 6&times;3

Connect the Universal module by the `A-Z` connector to the pad with `North`
orientation at position:
```
O O O O O O
X O O O O O
O O O O O O
```

```{note}
You can also try to create the configuration in code and serialize it using the library.
```


## Task 2: Create Hello World for Universal Module

Create a file `module_code.cpp` that will print `"Hello world {id}!"`
with the local module's id to the standard output.

```{tip}
Try running more clients (your module executable) and see what it does.<br>
When the client seems to be cycling, it probably couldn't be assigned any module.
```


## Task 3: Move Universal Module

Modify `module_code.cpp` such that the Universal module will move to the position:
```
O O O O O O
O O O O O X
O O O O O O
```

The module should be connected by connector `B-Z` with `North` orientation.

```{tip}
The default position of `rofi::hal::Connector` is extended.

So you don't need to extend the connectors of Pad,
but you need to retract a connector before connecting.
```

````{note}
The RoFI HAL is asynchronous by design.
However this tutorial is not about asynchronous programming,
so you can use {class}`std::promise` to synchronize the calls.
```{caution}
There is no guarantee that the thread in which the callback is called will end.
So use {func}`std::promise::set_value` instead of
{func}`std::promise::set_value_at_thread_exit`.
```
````


## Task 4: Multiple Modules with Same Code

Create a second Universal module with id `99` connected to the Pad at position Y:
```
O O O O O O
X O O O O Y
O O O O O O
```

Update the code in such a way so that both Universal modules meet at the middle
and connect to each other and print the connection orientation.

The Universal modules should be connected together with the `A-Z` connectors
and they should be connected to the pad at the following positions:
```
O O O O O O
O X O O Y O
O O O O O O
```

Try to program the module code in such a way that it works no matter what ids
the modules have.

```{hint}
You may need to think about the orientation in which to connect the second
Universal module to the pad in the configuration.
```


## Task 5: Module Communication

Update the code in such a way so that after both Universal modules meet
in the middle, they:
 1. Select a direction to a side.

 2. Disconnect from each other.

 3. Move in the selected direction.

    ```{hint}
    You will need to rotate with the gamma joint.
    ```

 4. Connect to each other at the new position.

    The Universal modules should be again connected together with the `A-Z` connectors.
    They should now be connected to the pad **right next to each other**.

Again, try to program the module code in such a way that it works no matter
what ids the modules have.


## Task 6: Packet Filter

Create a packet filter `discard_half.py` that discards every second packet
(counted for each sender module separately).

Create a configuration and code to test that it works.
