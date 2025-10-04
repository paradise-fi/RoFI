#pragma once

enum FunctionType
{
    Regular, // Registers the function as a regular function
    Barrier, // Registers the function as a barrier function
    Initial, // Registers the function as an initial function. There can only be one initial function.
};