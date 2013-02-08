/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace kinect;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
}

Task::~Task()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.




bool Task::configureHook()
{
    
    if (! RTT::TaskContext::configureHook())
        return false;
    

    

    
    return true;
    
}



bool Task::startHook()
{
    
    if (! RTT::TaskContext::startHook())
        return false;
    

    

    
    return true;
    
}



void Task::updateHook()
{
    
    RTT::TaskContext::updateHook();
    

    

    
}



void Task::errorHook()
{
    
    RTT::TaskContext::errorHook();
    

    

    
}



void Task::stopHook()
{
    
    RTT::TaskContext::stopHook();
    

    

    
}



void Task::cleanupHook()
{
    
    RTT::TaskContext::cleanupHook();
    

    

    
}

