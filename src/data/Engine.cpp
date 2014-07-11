/*
 * Temporary class to get i-score working with Score and Modular
 * Copyright © 2013, Théo de la Hogue
 *
 * License: This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */

#include "Engine.h"

#include <stdio.h>
#include <math.h>

using namespace std;

/*!
 * \file Engine.cpp
 * \author Théo de la Hogue, based on Engine.cpp written by Raphael Marczak (LaBRI) for the libIscore.
 * \date 2012-2013
 */

EngineCacheElement::EngineCacheElement()
{
    ;
}

EngineCacheElement::~EngineCacheElement()
{
    ;
}

Engine::Engine(void(*timeEventStatusAttributeCallback)(ConditionedProcessId, bool),
               void(*timeProcessSchedulerRunningAttributeCallback)(TimeProcessId, bool),
               void(*transportDataValueCallback)(TTSymbol&, const TTValue&),
               void (*networkDeviceNamespaceCallback)(TTSymbol&),
               std::string pathToTheJamomaFolder)
{
    m_TimeEventStatusAttributeCallback = timeEventStatusAttributeCallback;
    m_TimeProcessSchedulerRunningAttributeCallback = timeProcessSchedulerRunningAttributeCallback;
    m_TransportDataValueCallback = transportDataValueCallback;
    m_NetworkDeviceNamespaceCallback = networkDeviceNamespaceCallback;
    
    m_nextTimeProcessId = 1;
    m_nextIntervalId = 1;
    m_nextConditionedProcessId = 1;
    
    iscore = TTSymbol("i-score");
    
    if (!pathToTheJamomaFolder.empty()){
        
        initModular(pathToTheJamomaFolder.c_str());
        initScore(pathToTheJamomaFolder.c_str());
    }
    else {
        
        initModular();
        initScore();
    }
}

void Engine::initModular(const char* pathToTheJamomaFolder)
{
    TTErr    err;
    TTValue  args, out;
    TTString configFile = "/usr/local/include/IScore/i-scoreConfiguration.xml";     // TODO : declare as global variable
    
    TTLogMessage("\n*** Initialisation of Modular environnement ***\n");
    ////////////////////////////////////////////////////////////////////////

    // this initializes the Modular framework and loads protocol plugins
    TTModularInit(pathToTheJamomaFolder);
    
    // create application manager
    m_applicationManager = TTObject("ApplicationManager");
    
    
    TTLogMessage("\n*** Creation of i-score application ***\n");
    ////////////////////////////////////////////////////////////////////////////////
    
    // create a local application called "i-score" and get it back
    err = m_applicationManager.send("ApplicationInstantiateLocal", iscore, out);
    
    if (err) {
        TTLogError("Error : can't create i-score application \n");
        return;
    }
    else
        m_iscore = out[0];
    
    // set the application in debug mode
    m_iscore.set("debug", YES);
    
    // create a sender to send message to any application
    m_sender = TTObject("Sender");
    
    registerIscoreToProtocols();
}

void Engine::registerIscoreToProtocols()
{
    TTErr     err;
    TTValue   out;
    
    TTLogMessage("\n*** Enable Minuit communication ***\n");
    ////////////////////////////////////////////////////////////////////////
    
    // create a Minuit protocol unit
    err = m_applicationManager.send("ProtocolInstantiate", "Minuit", out);
    
    if (err) {
        TTLogError("Error : can't create Minuit protocol unit \n");
        return;
    }
    else
        m_protocolMinuit = out[0];
    
    // register i-score to the Minuit protocol
    m_protocolMinuit.send("ApplicationRegister", iscore, out);
    
    // select i-score to set its Minuit protocol parameters
    m_protocolMinuit.send("ApplicationSelect", iscore, out);
    m_protocolMinuit.set("port", MINUIT_INPUT_PORT);
    m_protocolMinuit.set("ip", "127.0.0.1");

    // launch Minuit protocol communication
    m_protocolMinuit.send("Run");
    
    TTLogMessage("\n*** Enable OSC communication ***\n");
    ////////////////////////////////////////////////////////////////////////
    
    // create a OSC protocol unit
    err = m_applicationManager.send("ProtocolInstantiate", "OSC", out);
    
    if (err) {
        TTLogError("Error : can't create OSC protocol unit \n");
        return;
    }
    else
        m_protocolOSC = out[0];
    
    // register i-score to the OSC protocol
    m_protocolOSC.send("ApplicationRegister", iscore, out);
    
    // select i-score to set its OSC protocol parameters
    m_protocolOSC.send("ApplicationSelect", iscore, out);
    m_protocolOSC.set("port", OSC_INPUT_PORT);
    m_protocolOSC.set("ip", "127.0.0.1");
    
    // launch OSC protocol communication
    m_protocolOSC.send("Run");
}

void Engine::initScore(const char* pathToTheJamomaFolder)
{   
    TTValue     args, out;
    
    TTLogMessage("\n*** Initialisation of Score environnement ***\n");
    /////////////////////////////////////////////////////////////////////
    
    // Init the Score library (passing the folder path where all the dylibs are)
    TTScoreInit(pathToTheJamomaFolder);
    
    
    TTLogMessage("\n*** Creating an empty Scenario ***\n");
    /////////////////////////////////////////////////////////////////////
    
    // Create an empty Scenario
    m_mainScenario = TTObject("Scenario");
    m_mainScenario.set("name", TTSymbol("Main"));

    // Store the main scenario (so ROOT_BOX_ID is 1)
    TTAddress address("/Main");
    cacheTimeProcess(m_mainScenario, address, m_mainScenario);
}

void Engine::dumpAddressBelow(TTNodePtr aNode)
{
    TTList returnedChildren;
    TTAddress anAddress;
    
    // fill a TTList with all children (because we use * (wilcard) for the name and the instance)
    aNode->getChildren(S_WILDCARD, S_WILDCARD, returnedChildren);
    
    // for each child
    for (returnedChildren.begin(); returnedChildren.end(); returnedChildren.next()) {
        // get a node from the TTList
        aNode = TTNodePtr((TTPtr)returnedChildren.current()[0]);
        
        // compute his absolute address
        aNode->getAddress(anAddress);
        
        std::cout << "   " << anAddress.string() << std::endl;
        
        // dump all addresses below this node (recursive call)
        dumpAddressBelow(aNode);
    }
}

Engine::~Engine()
{
    // Clear all the EngineCacheMaps
    // note : this should be useless because all elements are removed by the maquette
    clearTimeProcess();
    clearInterval();
    clearConditionedProcess();
    clearTimeCondition();
}

TimeProcessId Engine::cacheTimeProcess(TTObject& timeProcess, TTAddress& anAddress, TTObject& subScenario)
{
    TimeProcessId id;
    EngineCacheElementPtr e;
    
    e = new EngineCacheElement();
    e->object = timeProcess;
    e->address = anAddress;
    e->subScenario = subScenario;
    
    TTValue out, args = TTValue(e->address, e->object);
    m_iscore.send("RegisterObject", args, out);
    
    id = m_nextTimeProcessId;
    m_timeProcessMap[id] = e;
    m_nextTimeProcessId++;
    
    cacheStartCallback(id);
    cacheEndCallback(id);
    
    return id;
}

TTObject& Engine::getTimeProcess(TimeProcessId boxId)
{
    return m_timeProcessMap[boxId]->object;
}

TTAddress& Engine::getAddress(TimeProcessId boxId)
{
    return m_timeProcessMap[boxId]->address;
}

TTObject& Engine::getSubScenario(TimeProcessId boxId)
{
    return m_timeProcessMap[boxId]->subScenario;
}

TimeProcessId Engine::getParentId(TimeProcessId boxId)
{
    EngineCacheMapIterator it;
    TimeProcessId   parentId = NO_ID;
    TTObject        parentScenario;
    TTValue         v;
    
    m_timeProcessMap[boxId]->object.get("container", v);
    parentScenario = v[0];
    
    // retreive the id of the scenario
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        if (it->second->subScenario == parentScenario) {

            parentId = it->first;
            break;
        }
    }
    
    return parentId;
}

void Engine::getChildrenId(TimeProcessId boxId, vector<TimeProcessId>& childrenId)
{
    EngineCacheMapIterator it;
    TTObject    subScenario = getSubScenario(boxId);
    TTObject    parentScenario;
    TTValue     v;
    
    // Get all boxes id that have the subScenario as parent
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        if (it->first == boxId)
            continue;
        
        it->second->object.get("container", v);
        parentScenario = v[0];
        
        if (parentScenario != NULL && parentScenario == subScenario)
            childrenId.push_back(it->first);
    }
}

void Engine::uncacheTimeProcess(TimeProcessId boxId)
{
    EngineCacheElementPtr e = m_timeProcessMap[boxId];
    
    uncacheStartCallback(boxId);
    uncacheEndCallback(boxId);
    
    TTValue out;
    m_iscore.send("UnregisterObject", e->address, out);
    
    delete e;
    m_timeProcessMap.erase(boxId);
}

void Engine::clearTimeProcess()
{
    EngineCacheMapIterator it;
    
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        // don't remove the root time process (the main scenario)
        if (it->first != ROOT_BOX_ID) {
            
            uncacheStartCallback(it->first);
            uncacheEndCallback(it->first);
            delete it->second;
        }
    }
    
    // don't clear the m_timeProcessMap (because it is not empty)
    m_startCallbackMap.clear();
    m_endCallbackMap.clear();
    
    // set the next id to 2 because the main scenario is registered with the 1 id
    m_nextTimeProcessId = 2;
}

IntervalId Engine::cacheInterval(TTObject& timeProcess)
{
    TimeProcessId id;
    EngineCacheElementPtr e;
    
    e = new EngineCacheElement();
    e->object = timeProcess;
    
    id = m_nextIntervalId;
    m_intervalMap[id] = e;
    m_nextIntervalId++;
    
    return id;
}

TTObject& Engine::getInterval(IntervalId relationId)
{
    return m_intervalMap[relationId]->object;
}

void Engine::uncacheInterval(IntervalId relationId)
{
    EngineCacheElementPtr e = m_intervalMap[relationId];
    
    delete e;
    m_intervalMap.erase(relationId);
}

void Engine::clearInterval()
{
    EngineCacheMapIterator it;
    
    for (it = m_intervalMap.begin(); it != m_intervalMap.end(); ++it)
        delete it->second;
    
    m_intervalMap.clear();
    
    m_nextIntervalId = 1;
}

ConditionedProcessId Engine::cacheConditionedProcess(TimeProcessId timeProcessId, TimeEventIndex controlPointId)
{
    TTObject                timeProcess = getTimeProcess(timeProcessId);
    TTObject                timeEvent;
    ConditionedProcessId    id;
    EngineCacheElementPtr   e;
    TTValue                 v;
    
    // Get start or end time event
    if (controlPointId == BEGIN_CONTROL_POINT_INDEX)
        timeProcess.get("startEvent", v);
    else
        timeProcess.get("endEvent", v);
    
    timeEvent = v[0];
    
    // Create a new engine cache element
    e = new EngineCacheElement();
    e->object = timeProcess;
    e->index = controlPointId;
    
    id = m_nextConditionedProcessId;
    m_conditionedProcessMap[id] = e;
    m_nextConditionedProcessId++;
    
    // We cache an observer on time event status attribute
    cacheStatusCallback(id, controlPointId);
    
    return id;
}

TTObject& Engine::getConditionedProcess(ConditionedProcessId triggerId, TimeEventIndex& controlPointId)
{
    controlPointId = m_conditionedProcessMap[triggerId]->index;
    
    return m_conditionedProcessMap[triggerId]->object;
}

void Engine::uncacheConditionedProcess(ConditionedProcessId triggerId)
{
    // Get the engine cache element
    EngineCacheElementPtr e = m_conditionedProcessMap[triggerId];
    
    // Uncache observer on time event status attribute
    uncacheStatusCallback(triggerId, e->index);
    
    // Delete the engine cache element
    delete e;
    m_conditionedProcessMap.erase(triggerId);
}

void Engine::clearConditionedProcess()
{
    EngineCacheMapIterator it;
    
    for (it = m_conditionedProcessMap.begin(); it != m_conditionedProcessMap.end(); ++it) {
        
        uncacheStatusCallback(it->first, it->second->index);
        
        delete it->second;
    }
    
    m_conditionedProcessMap.clear();
    m_statusCallbackMap.clear();
    
    m_nextConditionedProcessId = 1;
}

void Engine::cacheTimeCondition(ConditionedProcessId triggerId, TTObject& timeCondition)
{
    EngineCacheElementPtr   e;
    TTValue                 args, v;
    
    e = new EngineCacheElement();
    e->object = timeCondition;
    e->index = triggerId;
    
    m_timeConditionMap[triggerId] = e;
}

TTObject& Engine::getTimeCondition(ConditionedProcessId triggerId)
{
    return m_timeConditionMap[triggerId]->object;
}

void Engine::uncacheTimeCondition(ConditionedProcessId triggerId)
{
    EngineCacheElementPtr   e = m_timeConditionMap[triggerId];
    
    delete e;
    m_timeConditionMap.erase(triggerId);
}

void Engine::clearTimeCondition()
{
    EngineCacheMapIterator it;
    
    for (it = m_timeConditionMap.begin(); it != m_timeConditionMap.end(); ++it)
        delete it->second;
    
    m_timeConditionMap.clear();
}

void Engine::cacheStartCallback(TimeProcessId boxId)
{
    EngineCacheElementPtr   e;
    TTValue                 baton;
    
    e = new EngineCacheElement();
    e->index = boxId;
    
    // create a TTCallback to observe when time process starts (using TimeProcessStartCallback)
    e->object = TTObject("callback");
    
    baton = TTValue(TTPtr(this), TTUInt32(boxId));
    e->object.set("baton", baton);
    e->object.set("function", TTPtr(&TimeProcessStartCallback));
    e->object.set("notification", TTSymbol("ProcessStarted"));
    
    // observe the "ProcessStarted" notification
    getTimeProcess(boxId).registerObserverForNotifications(e->object);

    m_startCallbackMap[boxId] = e;
}

void Engine::uncacheStartCallback(TimeProcessId boxId)
{
    EngineCacheElementPtr   e = m_startCallbackMap[boxId];
    TTValue                 v;
    
    // don't observe the "ProcessStarted" notification anymore
    getTimeProcess(boxId).unregisterObserverForNotifications(e->object);
    
    delete e;
    m_startCallbackMap.erase(boxId);
}

void Engine::cacheEndCallback(TimeProcessId boxId)
{
    EngineCacheElementPtr   e;
    TTValue                 baton;
    
    e = new EngineCacheElement();
    e->index = boxId;
    
    // create a TTCallback to observe when time process starts (using TimeProcessEndCallback)
    e->object = TTObject("callback");
    
    baton = TTValue(TTPtr(this), TTUInt32(boxId));
    e->object.set("baton", baton);
    e->object.set("function", TTPtr(&TimeProcessEndCallback));
    e->object.set("notification", TTSymbol("ProcessEnded"));
    
    // observe the "ProcessStarted" notification
    getTimeProcess(boxId).registerObserverForNotifications(e->object);
    
    m_endCallbackMap[boxId] = e;
}

void Engine::uncacheEndCallback(TimeProcessId boxId)
{
    EngineCacheElementPtr   e = m_endCallbackMap[boxId];
    TTValue                 v;
    
    // don't observe the "ProcessEnded" notification anymore
    getTimeProcess(boxId).registerObserverForNotifications(e->object);
    
    delete e;
    m_endCallbackMap.erase(boxId);
}

void Engine::cacheStatusCallback(ConditionedProcessId triggerId, TimeEventIndex controlPointId)
{
    EngineCacheElementPtr   e;
    TTObject    timeProcess = getConditionedProcess(triggerId, controlPointId);
    TTObject    timeEvent;
    TTValue     v, baton;
    
    // Create a new engine cache element
    e = new EngineCacheElement();
    e->index = triggerId;
    
    // get start or end time event
    if (controlPointId == BEGIN_CONTROL_POINT_INDEX)
        timeProcess.get("startEvent", v);
    else
        timeProcess.get("endEvent", v);
    
    timeEvent = v[0];
    
    // create a TTCallback to observe time event status attribute (using TimeEventStatusAttributeCallback)
    e->object = TTObject("callback");
    
    baton = TTValue(TTPtr(this), TTUInt32(triggerId));
    e->object.set("baton", baton);
    e->object.set("function", TTPtr(&TimeEventStatusAttributeCallback));
    e->object.set("notification", TTSymbol("EventStatusChanged"));
    
    // observe the "EventReadyChanged" notification
    timeEvent.registerObserverForNotifications(e->object);

    m_statusCallbackMap[triggerId] = e;
}

void Engine::uncacheStatusCallback(ConditionedProcessId triggerId, TimeEventIndex controlPointId)
{
    EngineCacheElementPtr   e = m_statusCallbackMap[triggerId];
    TTObject    timeProcess = getConditionedProcess(triggerId, controlPointId);
    TTObject    timeEvent;
    TTValue     v;
    
    // get start or end time event
    if (controlPointId == BEGIN_CONTROL_POINT_INDEX)
        timeProcess.get("startEvent", v);
    else
        timeProcess.get("endEvent", v);
    
    timeEvent = v[0];
    
    // don't observe the "EventStatusChanged" notification anymore
    timeEvent.unregisterObserverForNotifications(e->object);
    
    delete e;
    m_statusCallbackMap.erase(triggerId);
}

// Edition ////////////////////////////////////////////////////////////////////////

TimeProcessId Engine::addBox(TimeValue boxBeginPos, TimeValue boxLength, const std::string & name, TimeProcessId motherId)
{
    TTObject        startEvent, endEvent;
    TTObject        timeProcess, subScenario;
    TimeProcessId   boxId;
    TTAddress       address;
    TTValue         args, out;
    
    // create a time event for the start into the mother scenario
    getSubScenario(motherId).send("TimeEventCreate", boxBeginPos, out);
    startEvent = out[0];

    // create a time event for the end into the mother scenario
    getSubScenario(motherId).send("TimeEventCreate", boxBeginPos+boxLength, out);
    endEvent = out[0];
    
    // create a new automation time process into the mother scenario
    args = TTValue(TTSymbol("Automation"), startEvent, endEvent);
    getSubScenario(motherId).send("TimeProcessCreate", args, out);
    timeProcess = out[0];
    timeProcess.set("name", TTSymbol(name));
    
    // create a new sub scenario time process into the mother scenario
    args = TTValue(TTSymbol("Scenario"), startEvent, endEvent);
    getSubScenario(motherId).send("TimeProcessCreate", args, out);
    subScenario = out[0];
    
    // cache it and get an unique id for this process
    if (motherId == ROOT_BOX_ID)
        address = kTTAdrsRoot.appendAddress(TTAddress(name.data()));
    else
        address = getAddress(motherId).appendAddress(TTAddress(name.data()));
    
    boxId = cacheTimeProcess(timeProcess, address, subScenario);
    
    iscoreEngineDebug TTLogMessage("TimeProcess %ld created at %ld ms for a duration of %ld ms\n", boxId, boxBeginPos, boxLength);
    
	return boxId;
}

void Engine::removeBox(TimeProcessId boxId)
{
    TTObject    timeProcess, subScenario;
    TTObject    parentScenario;
    TTObject    startEvent, endEvent;
    TTValue     out;
    TTErr       err;
    
    // retreive the time process using the boxId
    timeProcess = getTimeProcess(boxId);
    subScenario = getSubScenario(boxId);
    
    // get the parent scenario
    timeProcess.get("container", out);
    parentScenario = out[0];
    
    // remove the time process from the cache
    uncacheTimeProcess(boxId);
    
    // release the time process from the mother scenario
    parentScenario.send("TimeProcessRelease", timeProcess, out);
    startEvent = out[0];
    endEvent = out[1];

    // release the sub scenario from the mother scenario
    parentScenario.send("TimeProcessRelease", subScenario, out);
    
    // release start event from the mother scenario
    err = parentScenario.send("TimeEventRelease", startEvent, out);
    if (err)
        iscoreEngineDebug TTLogMessage("Box %ld cannot release his start event\n", boxId);
    
    // release end event from the mother scenario
    err = parentScenario.send("TimeEventRelease", endEvent, out);
    if (err)
        iscoreEngineDebug TTLogMessage("Box %ld cannot release his end event\n", boxId);
}

IntervalId Engine::addTemporalRelation(TimeProcessId boxId1,
                                       TimeEventIndex controlPoint1,
                                       TimeProcessId boxId2,
                                       TimeEventIndex controlPoint2,
                                       TemporalRelationType type,
                                       vector<TimeProcessId>& movedBoxes)
{
    TTObject    timeProcess;
    TTObject    tp1, tp2;
    TTObject    startEvent, endEvent;
    TTObject    startScenario, endScenario;
    IntervalId  relationId;
    TTValue     args, out;
    TTErr       err;
    EngineCacheMapIterator  it;
    
    // get the events from the given box ids and pass them to the time process
    tp1 = getTimeProcess(boxId1);
    tp2 = getTimeProcess(boxId2);
    
    if (controlPoint1 == BEGIN_CONTROL_POINT_INDEX)
        tp1.get("startEvent", out);
    else
        tp1.get("endEvent", out);
    
    startEvent = out[0];
    
    if (controlPoint2 == BEGIN_CONTROL_POINT_INDEX)
        tp2.get("startEvent", out);
    else
        tp2.get("endEvent", out);
    
    endEvent = out[0];
    
    // get start and end events container
    startEvent.get("container", out);
    startScenario = out[0];
    
    endEvent.get("container", out);
    endScenario = out[0];
    
    // can't create a relation between to events of 2 differents scenarios
    if (startScenario != endScenario)
        return NO_ID;

    // create a new interval time process into the main scenario
    args = TTValue(TTSymbol("Interval"), startEvent, endEvent);
    err = startScenario.send("TimeProcessCreate", args, out);
    
    // an error can append if the start is after the end
    if (!err) {
        
        timeProcess = out[0];
        
        // set the time process rigid
        timeProcess.set("rigid", true);

        // cache it and get an unique id for this process
        relationId = cacheInterval(timeProcess);
    
        // return the entire timeProcessMap except the first process !!! (this is bad but it is like former engine)
        it = m_timeProcessMap.begin();
        it++;
        for (; it != m_timeProcessMap.end(); ++it)
            movedBoxes.push_back(it->first);
    
        return relationId;
    }
    else 
        return NO_ID;
}

void Engine::removeTemporalRelation(IntervalId relationId)
{
    TTObject    timeProcess;
    TTObject    startEvent, endEvent;
    TTObject    parentScenario;
    TTValue     out;
    TTErr       err;
    
    // retreive the interval using the relationId
    timeProcess = getInterval(relationId);
    
    // get the parent scenario
    timeProcess.get("container", out);
    parentScenario = out[0];
    
    // release the time process from the mother scenario
    parentScenario.send("TimeProcessRelease", timeProcess, out);
    startEvent = out[0];
    endEvent = out[1];
    
    // release start event from the mother scenario
    err = parentScenario.send("TimeEventRelease", startEvent, out);
    if (err)
        iscoreEngineDebug TTLogMessage("Relation %ld cannot release his start event\n", relationId);

    // release end event from the mother scenario
   err = parentScenario.send("TimeEventRelease", endEvent, out);
    if (err)
        iscoreEngineDebug TTLogMessage("Relation %ld cannot release his end event\n", relationId);

    // remove the interval from the cache
    uncacheInterval(relationId);
}

void Engine::changeTemporalRelationBounds(IntervalId relationId, BoundValue minBound, BoundValue maxBound, vector<TimeProcessId>& movedBoxes)
{
    TTObject    timeProcess = getInterval(relationId);
    TTUInt32    durationMin;
    TTUInt32    durationMax;
    TTValue     args, out;
    EngineCacheMapIterator  it;

    // filtering NO_BOUND (-1) and negative value because we use unsigned int
    if (minBound == NO_BOUND)
        durationMin = 0;
    else if (minBound < 0)
        durationMin = abs(minBound);
    else
        durationMin = minBound;
    
    if (maxBound == NO_BOUND)
        durationMax = 0;
    else if (maxBound < 0)
        durationMax = abs(maxBound);
    else
        durationMax = maxBound;
    
    // NOTE : sending 0 0 means the relation is not rigid
    // NOTE : it is also possible to use the "rigid" attribute to swicth between those two states
    args = TTValue(durationMin, durationMax);
    timeProcess.send("Limit", args, out);
    
    // return the entire timeProcessMap except the first process !!! (this is bad but it is like former engine)
    it = m_timeProcessMap.begin();
    it++;
    for (; it != m_timeProcessMap.end(); ++it)
        movedBoxes.push_back(it->first);
}

bool Engine::isTemporalRelationExisting(TimeProcessId boxId1, TimeEventIndex controlPoint1, TimeProcessId boxId2, TimeEventIndex controlPoint2)
{
    TTValue     v1, v2, v;
    IntervalId  found = NO_ID;
    EngineCacheMapIterator  it;
    
    // Get the events from the given box ids and pass them to the time process
    if (controlPoint1 == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId1).get("startEvent", v1);
    else
        getTimeProcess(boxId1).get("endEvent", v1);
    
    if (controlPoint2 == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId2).get("startEvent", v2);
    else
        getTimeProcess(boxId2).get("endEvent", v2);
    
    // Look into the interval map to retreive an interval with the same events
    for (it = m_intervalMap.begin(); it != m_intervalMap.end(); ++it) {
        
        it->second->object.get("startEvent", v);
        
        if (v == v1) {
            
            it->second->object.get("endEvent", v);
            
            if (v == v2) {
                found = it->first;
                break;
            }
        }
    }
    
	return found != NO_ID;
}

TimeProcessId Engine::getRelationFirstBoxId(IntervalId relationId)
{
    TTValue         out, v;
    TimeProcessId   found = NO_ID;
    EngineCacheMapIterator  it;
    
    // get the start event of the interval
	getInterval(relationId).get("startEvent", out);
    
    // Look into the automation map to retreive an automation with the same event
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        it->second->object.get("startEvent", v);
        
        if (out == v) {
            found = it->first;
            break;
        }
        
        it->second->object.get("endEvent", v);
        
        if (out == v) {
            found = it->first;
            break;
        }
    }
    
    return found;
}

TimeEventIndex Engine::getRelationFirstCtrlPointIndex(IntervalId relationId)
{
    TTValue         out, v;
    TimeEventIndex  ctrlPointId = NO_ID;
    EngineCacheMapIterator  it;
    
    // get the start event of the interval
	getInterval(relationId).get("startEvent", out);

    // Look into the automation map to retreive an automation with the same event
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        it->second->object.get("startEvent", v);
        
        if (out == v) {
            ctrlPointId = BEGIN_CONTROL_POINT_INDEX;
            break;
        }
        
        it->second->object.get("endEvent", v);
        
        if (out == v) {
            ctrlPointId = END_CONTROL_POINT_INDEX;
            break;
        }
    }
    
    return ctrlPointId;
}

TimeProcessId Engine::getRelationSecondBoxId(IntervalId relationId)
{
    TTValue         out, v;
    TimeProcessId   found = NO_ID;
    EngineCacheMapIterator  it;
    
    // Get the end event of the interval
	getInterval(relationId).get("endEvent", out);
    
    // Look into the automation map to retreive an automation with the same event
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        it->second->object.get("startEvent", v);
        
        if (out == v) {
            found = it->first;
            break;
        }
        
        it->second->object.get("endEvent", v);
        
        if (out == v) {
            found = it->first;
            break;
        }
    }
    
    return found;
}

TimeEventIndex Engine::getRelationSecondCtrlPointIndex(IntervalId relationId)
{
    TTValue         out, v;
    TimeEventIndex  ctrlPointId = NO_ID;
    EngineCacheMapIterator  it;
    
    // get the start event of the interval
	getInterval(relationId).get("endEvent", out);
    
    // Look into the automation map to retreive an automation with the same event
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        it->second->object.get("startEvent", v);
        
        if (out == v) {
            ctrlPointId = BEGIN_CONTROL_POINT_INDEX;
            break;
        }
        
        it->second->object.get("endEvent", v);
        
        if (out == v) {
            ctrlPointId = END_CONTROL_POINT_INDEX;
            break;
        }
    }
    
    return ctrlPointId;
}

BoundValue Engine::getRelationMinBound(IntervalId relationId)
{
    TTValue v;
    
    getInterval(relationId).get("durationMin", v);
    
    return v[0];
}

BoundValue Engine::getRelationMaxBound(IntervalId relationId)
{
    TTValue v;
    
    getInterval(relationId).get("durationMax", v);
    
    return v[0];
}

bool Engine::performBoxEditing(TimeProcessId boxId, TimeValue start, TimeValue end, vector<TimeProcessId>& movedBoxes)
{
    TTValue args, out;
    TTErr   err;
    EngineCacheMapIterator  it;
    
    args = TTValue(start, end);
    err = getTimeProcess(boxId).send("Move", args, out);

    // return the entire timeProcessMap except the first process !!! (this is bad but it is like former engine)
    it = m_timeProcessMap.begin();
    it++;
    for (; it != m_timeProcessMap.end(); ++it)
        movedBoxes.push_back(it->first);
    
    return !err;
}

std::string Engine::getBoxName(TimeProcessId boxId)
{
    TTSymbol    name;
    
	getTimeProcess(boxId).get("name", name);
    
    return name.c_str();
}

unsigned int Engine::getBoxVerticalPosition(TimeProcessId boxId)
{
    TTValue v;
    
	getTimeProcess(boxId).get("verticalPosition", v);
    
    return v[0];
}

void Engine::setBoxVerticalPosition(TimeProcessId boxId, unsigned int newPosition)
{
	getTimeProcess(boxId).set("verticalPosition", TTUInt32(newPosition));
}

unsigned int Engine::getBoxVerticalSize(TimeProcessId boxId)
{
    TTValue v;
    
	getTimeProcess(boxId).get("verticalSize", v);
    
    return v[0];
}

void Engine::setBoxVerticalSize(TimeProcessId boxId, unsigned int newSize)
{
    getTimeProcess(boxId).set("verticalSize", TTUInt32(newSize));
}

QColor Engine::getBoxColor(TimeProcessId boxId)
{
    TTValue v;
    QColor  color;
    
	getTimeProcess(boxId).get("color", v);
    
    color = QColor(v[0], v[1], v[2]);
    
    return color;
}

void Engine::setBoxColor(TimeProcessId boxId, QColor newColor)
{
    TTValue v;
    
    v = newColor.red();
    v.append(newColor.green());
    v.append(newColor.blue());
    
	getTimeProcess(boxId).set("color", v);
}

void Engine::setBoxMuteState(TimeProcessId boxId, bool muteState)
{
    getTimeProcess(boxId).set("mute", muteState);
}

bool Engine::getBoxMuteState(TimeProcessId boxId)
{
    TTValue v;
    
    getTimeProcess(boxId).get("mute", v);
    
    return TTBoolean(v[0]);
}

void Engine::setBoxName(TimeProcessId boxId, string name)
{
    TTSymbol    oldName, newName;
    
    getTimeProcess(boxId).get("name", oldName);
    
    newName = TTSymbol(name);
    
    // filter repetitions
    if (newName != oldName) {
        
        getTimeProcess(boxId).set("name", newName);
        
        // register the time process with the new name
        //unregisterObject(getAddress(boxId));
    }
}

TimeValue Engine::getBoxBeginTime(TimeProcessId boxId)
{
    TTValue     v;
    TimeValue   startDate;

	getTimeProcess(boxId).get("startDate", v);
    startDate = v[0];
    
    return startDate;
}

TimeValue Engine::getBoxEndTime(TimeProcessId boxId)
{
    TTValue     v;
    TimeValue   endDate;

	getTimeProcess(boxId).get("endDate", v);
    endDate = v[0];
    
    return endDate;
}

TimeValue Engine::getBoxDuration(TimeProcessId boxId)
{
    TTValue v;
    
	getTimeProcess(boxId).get("duration", v);
    
    return v[0];
}

int Engine::getBoxNbCtrlPoints(TimeProcessId boxId)
{
    TTValue v;
    
	getTimeProcess(boxId).get("intermediateEvents", v);
    
    return v.size() + 2; // because there is always a start and an end event too
}

TimeEventIndex Engine::getBoxFirstCtrlPointIndex(TimeProcessId boxId)
{
	return BEGIN_CONTROL_POINT_INDEX;
}

TimeEventIndex Engine::getBoxLastCtrlPointIndex(TimeProcessId boxId)
{
	return END_CONTROL_POINT_INDEX;
}

void Engine::setCtrlPointMessagesToSend(TimeProcessId boxId, TimeEventIndex controlPointIndex, std::vector<std::string> messageToSend, bool muteState)
{
    TTValue     out;
    TTObject    event;
    TTObject    state;
    TTUInt32    i;

    // get the start or end event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId).get("startEvent", out);
    else
        getTimeProcess(boxId).get("endEvent", out);
    
    event = out[0];
    
    // get the state of the event
    event.get("state", out);
    state = out[0];
    
    // clear the state
    state.send("Clear");
    
    // parse each incoming string into < directory:/address, value >
    for (i = 0; i < messageToSend.size(); i++) {
        
        TTValue v = TTString(messageToSend[i]);
        v.fromString();
        
        TTSymbol aSymbol = v[0];
		TTAddress anAddress = toTTAddress(aSymbol.string().data());
        v[0] = anAddress;
        
        // append a line to the state
        state.send("Append", v, out);
    }
    
    // flatten the state to increase the speed of the recall
    state.send("Flatten");
    
    // don't update curve for the root box because it is a Scenario and not an Automation
    if (boxId != ROOT_BOX_ID) {
    
        // update all curves (for automation process only)
        getTimeProcess(boxId).send("CurveUpdate");
    }
}

void Engine::getCtrlPointMessagesToSend(TimeProcessId boxId, TimeEventIndex controlPointIndex, std::vector<std::string>& messages)
{
    TTValue     out;
    TTObject    event;
    TTObject    state;
    TTBoolean   flattened;
    TTListPtr   lines = NULL;
    TTDictionaryBasePtr aLine;
    TTAddress   address;
    std::string s;
    
    // get the start or end event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId).get("startEvent", out);
    else
        getTimeProcess(boxId).get("endEvent", out);
    
    event = out[0];
    
    // get the state of the event
    event.get("state", out);
    state = out[0];
    
    // check if the state is flattened
    state.get("flattened", out);
    flattened = out[0];
    
    if (!flattened)
        state.send("Flatten");
    
    // get the state lines
    state.get("flattenedLines", out);
    lines = TTListPtr((TTPtr)out[0]);
    
    if (lines) {
        // edit each line address into a "directory/address value" string
        for (lines->begin(); lines->end(); lines->next()) {
            
            aLine = TTDictionaryBasePtr((TTPtr)lines->current()[0]);
            
            // get the target address
            aLine->lookup(kTTSym_target, out);
            address = out[0];
            
            // get value
            aLine->getValue(out);
            out.toString();
            
            // edit string
            s = toNetworkTreeAddress(address);
            s += " ";
            s += TTString(out[0]).c_str();
            
            messages.push_back(s);
        }
    }
}

void Engine::setCtrlPointMutingState(TimeProcessId boxId, TimeEventIndex controlPointIndex, bool mute)
{
    TTValue     out;
    TTObject    event;
    
    // get the start or end event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId).get("startEvent", out);
    else
        getTimeProcess(boxId).get("endEvent", out);
    
    event = out[0];
    
    event.set("mute", mute);
}

bool Engine::getCtrlPointMutingState(TimeProcessId boxId, TimeEventIndex controlPointIndex)
{
    TTValue     out;
    TTObject    event;
    
    // get the start or end event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        getTimeProcess(boxId).get("startEvent", out);
    else
        getTimeProcess(boxId).get("endEvent", out);
    
    event = out[0];
    
    event.get("mute", out);
    
    return TTBoolean(out[0]);
}

//CURVES ////////////////////////////////////////////////////////////////////////////////////

void Engine::addCurve(TimeProcessId boxId, const std::string & address)
{
    TTValue out;
    
    // add the curve addresses into the automation time process
    getTimeProcess(boxId).send("CurveAdd", toTTAddress(address), out);
}

void Engine::removeCurve(TimeProcessId boxId, const std::string & address)
{
    TTValue out;
    
    // remove the curve addresses of the automation time process
    getTimeProcess(boxId).send("CurveRemove", toTTAddress(address), out);
}

void Engine::clearCurves(TimeProcessId boxId)
{
    // clear all the curves of the automation time process
    getTimeProcess(boxId).send("Clear");
}

std::vector<std::string> Engine::getCurvesAddress(TimeProcessId boxId)
{
    std::vector<std::string> curveAddresses;
    TTValue out;
    
    // get the curve addresses of the automation time process
    getTimeProcess(boxId).get("curveAddresses", out);
    
    // copy the addresses into the vector
    for (TTUInt32 i = 0; i < out.size(); i++)
        curveAddresses.push_back(toNetworkTreeAddress(out[i]));
    
	return curveAddresses;
}

void Engine::setCurveSampleRate(TimeProcessId boxId, const std::string & address, unsigned int nbSamplesBySec)
{
    TTObject    curve;
    TTValue     objects;
    TTUInt32    i;
    TTErr       err;
    
    // get curve object at address
    err = getTimeProcess(boxId).send("CurveGet", toTTAddress(address), objects);
    
    if (!err) {
        
        // set each indexed curve
        for (i = 0; i < objects.size(); i++) {
            
            curve = objects[i];
            
            curve.set("sampleRate", nbSamplesBySec);
        }
    }
}

unsigned int Engine::getCurveSampleRate(TimeProcessId boxId, const std::string & address)
{
    TTObject    curve;
    TTValue     out, objects;
    TTErr       err;
    
    // get curve object at address
    err = getTimeProcess(boxId).send("CurveGet", toTTAddress(address), objects);
    
    if (!err) {
        
        curve = objects[0];
        
        curve.get("sampleRate", out);
        
        return TTUInt32(out[0]);
    }
    
	return 0;
}

void Engine::setCurveRedundancy(TimeProcessId boxId, const std::string & address, bool redundancy)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             objects;
    TTUInt32            i;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // set each indexed curve
        for (i = 0; i < objects.size(); i++) {
            
            curve = objects[i];
            
            curve->setAttributeValue(TTSymbol("redundancy"), redundancy);
        }
    }
}

bool Engine::getCurveRedundancy(TimeProcessId boxId, const std::string & address)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             v, objects;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // get first indexed curve only
        curve = objects[0];
        
        curve->getAttributeValue(TTSymbol("redundancy"), v);
        
        return TTBoolean(v[0]);
    }
    
	return false;
}

void Engine::setCurveMuteState(TimeProcessId boxId, const std::string & address, bool muteState)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             objects;
    TTUInt32            i;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // set each indexed curve
        for (i = 0; i < objects.size(); i++) {
        
            curve = objects[i];
        
            curve->setAttributeValue(TTSymbol("active"), !muteState);
        }
    }
}

bool Engine::getCurveMuteState(TimeProcessId boxId, const std::string & address)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             v, objects;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // get first indexed curve only
        curve = objects[0];
        
        curve->getAttributeValue(TTSymbol("active"), v);
        
        return !TTBoolean(v[0]);
    }
    
	return false;
}

void Engine::setCurveRecording(TimeProcessId boxId, const std::string & address, bool record)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTValue             v, out;
    
    // enable/disable recording
    v = toTTAddress(address);
    v.append(record);
    
    timeProcess->sendMessage(TTSymbol("CurveRecord"), v, out);
}

bool Engine::setCurveSections(TimeProcessId boxId, std::string address, unsigned int argNb, const std::vector<float> & percent, const std::vector<float> & y, const std::vector<short> & sectionType, const std::vector<float> & coeff)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             v, objects;
    TTUInt32            i, nbPoints = coeff.size();
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // edit value as : x1 y1 b1 x2 y2 b2
        v.resize(nbPoints * 3);
        
        for (i = 0; i < v.size(); i = i+3) {
            
            v[i] = TTFloat64(percent[i/3] / 100.);
            v[i+1] = TTFloat64(y[i/3]);
            v[i+2] = TTFloat64(coeff[i/3]) * TTFloat64(coeff[i/3]) * TTFloat64(coeff[i/3]) * TTFloat64(coeff[i/3]);
            
        }
        
        // set first indexed curve only
        curve = objects[0];
        
        // set a curve parameters
        err = curve->setAttributeValue(TTSymbol("functionParameters"), v);
    }
    
    return err == kTTErrNone;
}

bool Engine::getCurveSections(TimeProcessId boxId, std::string address, unsigned int argNb,
                              std::vector<float> & percent,  std::vector<float> & y,  std::vector<short> & sectionType,  std::vector<float> & coeff)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             v, objects;
    TTUInt32            i;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), objects);
    
    if (!err) {
        
        // get first indexed curve only
        curve = objects[0];

        // get a curve parameters
        err = curve->getAttributeValue(TTSymbol("functionParameters"), v);
        
        if (!err) {

            // edit percent, y, sectionType and coeff from v : x1 y1 b1 x2 y2 b2 . . .
            for (i = 0; i < v.size(); i = i+3) {
                
                percent.push_back(TTFloat64(v[i]) * 100.);
                y.push_back(TTFloat64(v[i+1]));
                sectionType.push_back(1);
                coeff.push_back(sqrt(sqrt(TTFloat64(v[i+2]))));
            }
        }
    }
    
    return err == kTTErrNone;
}

bool Engine::getCurveValues(TimeProcessId boxId, const std::string & address, unsigned int argNb, std::vector<float>& result)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(boxId);
    TTObjectBasePtr     curve;
    TTValue             v, curveValues;
    TTErr               err;
    
    // get curve object at address
    err = timeProcess->sendMessage(TTSymbol("CurveGet"), toTTAddress(address), v);

    if (!err) {
        
        // get first indexed curve only
        curve = v[0];
        
        // get time process duration
        timeProcess->getAttributeValue(TTSymbol("duration"), v);
    
        // sample the curve
        err = curve->sendMessage(TTSymbol("Sample"), v, curveValues);
    
        // copy the curveValues into the result vector
        for (TTUInt32 i = 0; i < curveValues.size(); i++)
            result.push_back(TTFloat64(curveValues[i]));
    }
    
	return err == kTTErrNone;
}

ConditionedProcessId Engine::addTriggerPoint(TimeProcessId containingBoxId, TimeEventIndex controlPointIndex)
{
    TTTimeProcessPtr        timeProcess = getTimeProcess(containingBoxId);
    TTTimeEventPtr          timeEvent;
    TTTimeConditionPtr      timeCondition;
    TTTimeContainerPtr      parentScenario;
    ConditionedProcessId    triggerId;
    TTValue                 v, args, out;
    TTString                instance;
    
    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);
    
    // get the parent scenario
    timeProcess->getAttributeValue(TTSymbol("container"), v);
    parentScenario = TTTimeContainerPtr(TTObjectBasePtr(v[0]));
    
    // Create a TTTimeCondition
    parentScenario->sendMessage(TTSymbol("TimeConditionCreate"), args, v);
    timeCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));
    
    // Add the event to the condition with no associated expression
    args = TTObjectBasePtr(timeEvent);
    timeCondition->sendMessage(TTSymbol("EventAdd"), args, v);
    
    // We cache the time process and the event index instead of the event itself
    triggerId = cacheConditionedProcess(containingBoxId, controlPointIndex);
    
    // We cache the TTTimeCondition
    cacheTimeCondition(triggerId, timeCondition);

    // note : see in setTriggerPointMessage to see how the expression associated to an event is edited
    
	return triggerId;
}

void Engine::removeTriggerPoint(ConditionedProcessId triggerId)
{
    // check existence before because they could have been destroyed in deleteCondition
    if (m_conditionedProcessMap.find(triggerId) == m_conditionedProcessMap.end())
        return;
    
    TTValue             v, out;
    TTTimeConditionPtr  timeCondition = getTimeCondition(triggerId);
    TTTimeContainerPtr  parentScenario;
    
    // get the parent scenario
    timeCondition->getAttributeValue(TTSymbol("container"), v);
    parentScenario = TTTimeContainerPtr(TTObjectBasePtr(v[0]));
    
    // Release the time condition
    v = TTObjectBasePtr(timeCondition);
    parentScenario->sendMessage(TTSymbol("TimeConditionRelease"), v, out);
    
    // Uncache
    uncacheConditionedProcess(triggerId);
    
    uncacheTimeCondition(triggerId);
}

TimeConditionId Engine::createCondition(std::vector<ConditionedProcessId> triggerIds)
{
    std::vector<ConditionedProcessId>::iterator it = triggerIds.begin();
    TTTimeConditionPtr timeCondition = getTimeCondition(*it);

    // Create an id for the condition and cache it (from ConditionedProcessId because it is mixed with timeConditionMap)
    TimeConditionId conditionId = m_nextConditionedProcessId++;
    cacheTimeCondition(conditionId, timeCondition);

    for(++it ; it != triggerIds.end() ; ++it) {
        attachToCondition(conditionId, *it);
    }

    return conditionId;
}

void Engine::attachToCondition(TimeConditionId conditionId, ConditionedProcessId triggerId)
{
    TTTimeConditionPtr  timeCondition = getTimeCondition(conditionId);
    TTTimeConditionPtr  otherCondition = getTimeCondition(triggerId);
    TimeEventIndex      idx = BEGIN_CONTROL_POINT_INDEX;                // Because a condition is always at the start of a box
    TTTimeEventPtr      timeEvent;
    TTTimeContainerPtr  parentScenario;
    TTValue             args, v;
    
    TTScoreTimeProcessGetStartEvent(getConditionedProcess(triggerId, idx), &timeEvent);

    // Should be different conditions before the merge
    if (otherCondition != timeCondition) {

        // Save the expression
        std::string expr = getTriggerPointMessage(triggerId);
        
        // get parent scenario
        timeEvent->getAttributeValue(TTSymbol("container"), v);
        parentScenario = TTTimeContainerPtr(TTObjectBasePtr(v[0]));

        // Release the other time condition
        parentScenario->sendMessage(TTSymbol("TimeConditionRelease"), otherCondition, v);

        // Add the event to the chosen time condition with the saved expression
        args = TTObjectBasePtr(timeEvent);
        timeCondition->sendMessage(TTSymbol("EventAdd"), args, v);
//        setTriggerPointMessage(triggerId, expr);

        // Modify the cache
        m_timeConditionMap[triggerId]->object = timeCondition;
        m_conditionsMap[conditionId].push_back(triggerId);
    }
}

void Engine::detachFromCondition(TimeConditionId conditionId, ConditionedProcessId triggerId)
{
    // check existence before because they could have been destroyed in removeTriggerPoint
    if (m_conditionsMap.find(conditionId) == m_conditionsMap.end())
        return;
    
    if (m_conditionedProcessMap.find(triggerId) == m_conditionedProcessMap.end())
        return;
    
    TTTimeConditionPtr  timeCondition = getTimeCondition(conditionId);
    TTTimeConditionPtr  otherCondition = getTimeCondition(triggerId);
    TimeEventIndex      idx = BEGIN_CONTROL_POINT_INDEX;                 // Because a condition is always at the start of a box
    TTTimeEventPtr      timeEvent;
    TTTimeContainerPtr  parentScenario;
    TTValue             args, v;
    
    TTScoreTimeProcessGetStartEvent(getConditionedProcess(triggerId, idx), &timeEvent);

    // Should be the same condition before the separation
    if (otherCondition == timeCondition) {

        // Save the expression
        std::string expr = getTriggerPointMessage(triggerId);
        
        // get parent scenario
        timeEvent->getAttributeValue(TTSymbol("container"), v);
        parentScenario = TTTimeContainerPtr(TTObjectBasePtr(v[0]));

        // Remove the event from the chosen time condition
        args = TTObjectBasePtr(timeEvent);
        timeCondition->sendMessage(TTSymbol("EventRemove"), args, v);

        // Create a new TTTimeCondition
        parentScenario->sendMessage(TTSymbol("TimeConditionCreate"), args, v);
        otherCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));

        // Add the event to the new condition with the saved expression
        timeCondition->sendMessage(TTSymbol("EventAdd"), args, v);
        setTriggerPointMessage(triggerId, expr);

        // Modify cache
        m_timeConditionMap[triggerId]->object = otherCondition;
        m_conditionsMap[conditionId].remove(triggerId);
    }
}

void Engine::deleteCondition(TimeConditionId conditionId)
{
    std::list<ConditionedProcessId> & triggerIds = m_conditionsMap[conditionId];
    std::list<ConditionedProcessId>::iterator it;
    TTTimeConditionPtr  timeCondition = getTimeCondition(conditionId);
    TTTimeContainerPtr  parentScenario;
    TTValue             v;

    for(it = triggerIds.begin() ; it != triggerIds.end() ; ++it) {
        detachFromCondition(conditionId, *it);
    }
    
    // get parent scenario
    timeCondition->getAttributeValue(TTSymbol("container"), v);
    parentScenario = TTTimeContainerPtr(TTObjectBasePtr(v[0]));

    // Release the condition
    parentScenario->sendMessage(TTSymbol("TimeConditionRelease"), timeCondition, v);

    // Uncache the condition
    uncacheTimeCondition(conditionId);
    m_conditionsMap.erase(conditionId);
}

void Engine::getConditionTriggerIds(TimeConditionId conditionId, std::vector<TimeProcessId>& triggerIds)
{
    triggerIds.assign(m_conditionsMap[conditionId].begin(), m_conditionsMap[conditionId].end());
}

void Engine::setConditionMessage(TimeConditionId conditionId, std::string disposeMessage)
{
    TTTimeConditionPtr timeCondition = getTimeCondition(conditionId);
    TTValue v;

    v.append(TTSymbol(disposeMessage));
    timeCondition->setAttributeValue(TTSymbol("disposeMessage"), v);
}

std::string Engine::getConditionMessage(TimeConditionId conditionId)
{
    TTTimeConditionPtr timeCondition = getTimeCondition(conditionId);
    TTValue out;

    timeCondition->getAttributeValue(TTSymbol("disposeMessage"), out);

    TTSymbol expr = out[0];
    return expr.c_str();
}

void Engine::setTriggerPointMessage(ConditionedProcessId triggerId, std::string triggerMessage)
{
    TimeEventIndex      controlPointIndex;
    TTTimeProcessPtr    timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    TTTimeEventPtr      timeEvent;
    TTValue             v, out;
    
    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);
    
    // edit the expression associated to this event
    v = TTObjectBasePtr(timeEvent);
    v.append(TTSymbol(triggerMessage));
    getTimeCondition(triggerId)->sendMessage(TTSymbol("EventExpression"), v, out);
}

std::string Engine::getTriggerPointMessage(ConditionedProcessId triggerId)
{
    TimeEventIndex          controlPointIndex;
    TTTimeProcessPtr        timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    TTTimeEventPtr          timeEvent;
    TTSymbol                expression;
    TTValue                 v, out;
    
    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);
    
    // Get the expression associated to this event
    v = TTObjectBasePtr(timeEvent);
    if (!getTimeCondition(triggerId)->sendMessage(TTSymbol("ExpressionFind"), v, out)) {
        
        expression = out[0];
        return expression.c_str();
    }
    else {
        
        string empty;
        return empty;
    }
}

//!\ Crappy copy
void Engine::setTriggerPointDefault(ConditionedProcessId triggerId, bool dflt)
{
    TimeEventIndex      controlPointIndex;
    TTTimeProcessPtr    timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    TTTimeEventPtr      timeEvent;
    TTValue             v, out;

    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);

    // edit the default comportment associated to this event
    v = TTObjectBasePtr(timeEvent);
    v.append(dflt);
    getTimeCondition(triggerId)->sendMessage(TTSymbol("EventDefault"), v, out);
}

//!\ Crappy copy
bool Engine::getTriggerPointDefault(ConditionedProcessId triggerId)
{
    TimeEventIndex          controlPointIndex;
    TTTimeProcessPtr        timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    TTTimeEventPtr          timeEvent;
    bool                    dflt;
    TTValue                 v, out;

    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);

    // Get the default comportment associated to this event
    v = TTObjectBasePtr(timeEvent);
    if (!getTimeCondition(triggerId)->sendMessage(TTSymbol("DefaultFind"), v, out)) {

        dflt = out[0];
        return dflt;
    }
    else {

        bool empty;
        return empty;
    }
}

TimeProcessId Engine::getTriggerPointRelatedBoxId(ConditionedProcessId triggerId)
{
    TimeEventIndex          controlPointIndex;
    TTTimeProcessPtr        timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    EngineCacheMapIterator  it;
    TimeProcessId           id = NO_ID;
    
    // look for the time process id into the time process map
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
        
        if (it->second->object == timeProcess) {
            
            id = it->first;
            break;
        }
    }

    return id;
}

TimeEventIndex Engine::getTriggerPointRelatedCtrlPointIndex(ConditionedProcessId triggerId)
{
    return m_conditionedProcessMap[triggerId]->index;;
}

void Engine::getBoxesId(vector<TimeProcessId>& boxesID)
{
    EngineCacheMapIterator it;
    
    boxesID.clear();
    
    for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it)
        boxesID.push_back(it->first);
}

void Engine::getRelationsId(vector<IntervalId>& relationsID)
{
    EngineCacheMapIterator it;
    
    relationsID.clear();
    
    for (it = m_intervalMap.begin(); it != m_intervalMap.end(); ++it)
        relationsID.push_back(it->first);
}

void Engine::getTriggersPointId(vector<ConditionedProcessId>& triggersID)
{
    EngineCacheMapIterator it;
    
    triggersID.clear();
    
    for (it = m_conditionedProcessMap.begin(); it != m_conditionedProcessMap.end(); ++it)
        triggersID.push_back(it->first);
}

void Engine::getConditionsId(vector<TimeConditionId>& conditionsID)
{
    std::map<TimeConditionId, std::list<ConditionedProcessId>>::iterator it;

    conditionsID.clear();

    for(it = m_conditionsMap.begin() ; it != m_conditionsMap.end() ; ++it) {
        conditionsID.push_back(it->first);
    }
}

void Engine::setViewZoom(QPointF zoom)
{
    TTValue v;
    
    v.append(zoom.x());
    v.append(zoom.y());
    
    m_mainScenario->setAttributeValue(TTSymbol("viewZoom"), v);
}

QPointF Engine::getViewZoom()
{
    QPointF zoom;
    TTValue v;
    
    m_mainScenario->getAttributeValue(TTSymbol("viewZoom"), v);
    
    zoom = QPointF(v[0], v[1]);
    
    return zoom;
}

void Engine::setViewPosition(QPointF position)
{
    TTValue v;
    
    v.append(position.x());
    v.append(position.y());
    
    m_mainScenario->setAttributeValue(TTSymbol("viewPosition"), v);
}

QPointF Engine::getViewPosition()
{
    QPointF position;
    TTValue v;
    
    m_mainScenario->getAttributeValue(TTSymbol("viewPosition"), v);
    
    position = QPointF(v[0], v[1]);
    
    return position;
}

//Execution ///////////////////////////////////////////////////////////
void Engine::setTimeOffset(TimeValue timeOffset, bool mute)
{
    TTValue v, none;
    
    v = timeOffset;
    v.append(mute);
    
    // set the time process at time offset (an optionaly mute the output)
    m_mainScenario->sendMessage(kTTSym_Goto, v, none);
    
    TTLogMessage("Engine::setTimeOffset = %ld\n", timeOffset);       
}

TimeValue Engine::getTimeOffset()
{
    TTValue         v;
    TTObjectBasePtr scheduler;
    
    // TODO : TTTimeProcess should extend Scheduler class
    m_mainScenario->getAttributeValue(TTSymbol("scheduler"), v);
    
    scheduler = v[0];
    
    scheduler->getAttributeValue(kTTSym_offset, v);
    
    TTLogMessage("Engine::getTimeOffset = %ld\n", TimeValue(TTFloat64(v[0])));
    
    return TimeValue(TTFloat64(v[0]));
}

bool Engine::play(TimeProcessId processId)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    TTTimeProcessPtr    subScenario = getSubScenario(processId);
    
    TTLogMessage("***************************************\n");
    TTLogMessage("Engine::play\n");
    
    // make the start event to happen
    TTErr err = timeProcess->sendMessage("Start");
    if (processId != ROOT_BOX_ID)
        subScenario->sendMessage("Start");
    
    return err == kTTErrNone;
}

bool Engine::isPlaying(TimeProcessId processId)
{
    TTValue             v;
    TTObjectBasePtr     aScheduler;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    // get the scheduler object of the main scenario
    timeProcess->getAttributeValue("scheduler", v);
    aScheduler = TTObjectBasePtr(v[0]);
    
    aScheduler->getAttributeValue("running", v);
    
    return TTBoolean(v[0]);
}

bool Engine::stop(TimeProcessId processId)
{
    TTValue             objects;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    TTTimeProcessPtr    subScenario = getSubScenario(processId);
    
    TTLogMessage("Engine::stop\n");
    
    // stop the main scenario execution
    // but the end event don't happen
    TTBoolean success = !timeProcess->sendMessage(kTTSym_Stop);
    if (processId != ROOT_BOX_ID)
        subScenario->sendMessage(kTTSym_Stop);
    
    if (processId == ROOT_BOX_ID) {
        // get all TTTimeProcesses
        timeProcess->getAttributeValue(TTSymbol("timeProcesses"), objects);
        
        // Stop all time process
        for (TTUInt32 i = 0; i < objects.size(); i++) {
            
            timeProcess = TTTimeProcessPtr(TTObjectBasePtr(objects[i]));
            
            timeProcess->sendMessage(kTTSym_Stop);
        }
    }
    
    TTLogMessage("***************************************\n");
    
    return success;
}

void Engine::pause(bool pauseValue, TimeProcessId processId)
{
    TTTimeProcessPtr  timeProcess = getTimeProcess(processId);
    TTTimeProcessPtr  subScenario = getSubScenario(processId);
    
    if (pauseValue) {
        TTLogMessage("---------------------------------------\n");
        timeProcess->sendMessage(kTTSym_Pause);
        if (processId != ROOT_BOX_ID)
            subScenario->sendMessage(kTTSym_Pause);
    }
    else {
        TTLogMessage("+++++++++++++++++++++++++++++++++++++++\n");
        timeProcess->sendMessage(kTTSym_Resume);
        if (processId != ROOT_BOX_ID)
            subScenario->sendMessage(kTTSym_Resume);
    }
}

bool Engine::isPaused(TimeProcessId processId)
{
    TTValue             v;
    TTObjectBasePtr     aScheduler;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    // get the scheduler object of the main scenario
    timeProcess->getAttributeValue(TTSymbol("scheduler"), v);
    aScheduler = TTObjectBasePtr(v[0]);
    
    aScheduler->getAttributeValue(TTSymbol("paused"), v);
    
    return TTBoolean(v[0]);
}

TimeValue Engine::getCurrentExecutionDate(TimeProcessId processId)
{
    TTValue             v;
    TTUInt32            time;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    timeProcess->getAttributeValue("date", v);
    time = TTFloat64(v[0]);
    
    return time;
}

float Engine::getCurrentExecutionPosition(TimeProcessId processId)
{
    TTValue             v;
    TTFloat64           position;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    timeProcess->getAttributeValue("position", v);
    position = TTFloat64(v[0]);
    
    return position;
}

void Engine::setExecutionSpeedFactor(float factor, TimeProcessId processId)
{
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    TTTimeProcessPtr    subScenario = getSubScenario(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    timeProcess->setAttributeValue(kTTSym_speed, TTFloat64(factor));
    if (processId != ROOT_BOX_ID)
        subScenario->setAttributeValue(kTTSym_speed, TTFloat64(factor));
}

float Engine::getExecutionSpeedFactor(TimeProcessId processId)
{
    TTValue             v;
    TTTimeProcessPtr    timeProcess = getTimeProcess(processId);
    
    // TODO : TTTimeProcess should extend Scheduler class
    timeProcess->getAttributeValue(kTTSym_speed, v);
    
    return TTFloat64(v[0]);
}

void Engine::trigger(ConditionedProcessId triggerId)
{
    TimeEventIndex      controlPointIndex;
    TTTimeProcessPtr    timeProcess = getConditionedProcess(triggerId, controlPointIndex);
    TTTimeConditionPtr  timeCondition;
    TTTimeEventPtr      timeEvent;
    TTValue             v, out;
    
    // Get start or end time event
    if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
        TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
    else
        TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);
    
    // Get event condition
    timeEvent->getAttributeValue("condition", v);
    timeCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));
    
    // Tell the condition to trigger this event (and dispose the others)
    v = TTObjectBasePtr(timeEvent);
    timeCondition->sendMessage("Trigger", v, out);
}

void Engine::trigger(vector<ConditionedProcessId> triggerIds)
{
    vector<ConditionedProcessId>::iterator it;
    TTTimeConditionPtr  lastTimeCondition = NULL;
    TTValue             v, events, out;
    
    // get all time events
    for (it = triggerIds.begin(); it != triggerIds.end(); ++it) {
        
        TimeEventIndex      controlPointIndex;
        TTTimeProcessPtr    timeProcess = getConditionedProcess(*it, controlPointIndex);
        TTTimeEventPtr      timeEvent;
        TTTimeConditionPtr  timeCondition;
        
        // Get start or end time event
        if (controlPointIndex == BEGIN_CONTROL_POINT_INDEX)
            TTScoreTimeProcessGetStartEvent(timeProcess, &timeEvent);
        else
            TTScoreTimeProcessGetEndEvent(timeProcess, &timeEvent);
        
        // Get event condition
        timeEvent->getAttributeValue("condition", v);
        timeCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));
        
        // check that all events are part of the same condition
        if (!lastTimeCondition)
            lastTimeCondition = timeCondition;
        
        if (lastTimeCondition != timeCondition) {
            
            lastTimeCondition = NULL;
            break;
        }
        
        events.append(TTObjectBasePtr(timeEvent));
    }
    
    // if all events are part of the same time condition
    if (lastTimeCondition)
        lastTimeCondition->sendMessage("Trigger", events, out);
}

void Engine::addNetworkDevice(const std::string & deviceName, const std::string & pluginToUse, const std::string & DeviceIp, const unsigned int & destinationPort, const unsigned int & receptionPort)
{
    TTValue         v, portValue, none;
    TTSymbol        applicationName(deviceName);
    TTSymbol        protocolName(pluginToUse);
    TTObjectBasePtr anApplication = NULL;
    TTHashPtr       hashParameters;
    TTErr           err;
    
    // if the application doesn't already exist
    if (!getApplication(applicationName)) {
        
        // create the application
        v = applicationName;
        TTObjectBaseInstantiate(kTTSym_Application, TTObjectBaseHandle(&anApplication), v);
        
        // set application type : here 'mirror' because it use Minuit protocol
        // note : this should be done for all protocols which have a discovery feature
        if (protocolName == TTSymbol("Minuit"))
            anApplication->setAttributeValue(kTTSym_type, TTSymbol("mirror"));
        
        // set application type : here 'proxy' because it use OSC protocol
        // note : this should be done for all protocols which have no discovery feature
        if (protocolName == TTSymbol("OSC"))
            anApplication->setAttributeValue(kTTSym_type, TTSymbol("proxy"));
        
        // check if the protocol has been loaded
		if (getProtocol(protocolName)) {
            
            // stop the protocol
            getProtocol(protocolName)->sendMessage(TTSymbol("Stop"));
            
            // register the application to the protocol
            v = applicationName;
            getProtocol(protocolName)->sendMessage(TTSymbol("registerApplication"), v, none);
            
            err = getProtocol(protocolName)->getAttributeValue(TTSymbol("applicationParameters"), v);
            
            if (!err) {
                
                hashParameters = TTHashPtr((TTPtr)v[0]);
            
                // set plugin parameters (OSC or Minuit Plugin)
                hashParameters->remove(TTSymbol("ip"));
                hashParameters->append(TTSymbol("ip"), TTSymbol(DeviceIp));
                
                portValue = destinationPort;
                if (receptionPort != 0)
                    portValue.append(receptionPort);
                
                hashParameters->remove(TTSymbol("port"));
                hashParameters->append(TTSymbol("port"), portValue);
            
                v = applicationName;
                v.append(TTPtr(hashParameters));
                getProtocol(protocolName)->setAttributeValue(TTSymbol("applicationParameters"), v);
            }
            
            // run the protocol
            getProtocol(protocolName)->sendMessage(TTSymbol("Run"));
        }
        
        // set the priority, service, tag and rangeBounds attributes as a cached attributes
        v = kTTSym_priority;
        v.append(kTTSym_service);
        v.append(kTTSym_tag);
        v.append(kTTSym_rangeBounds);
        v.append(kTTSym_rangeClipmode);
        anApplication->setAttributeValue(TTSymbol("cachedAttributes"), v);
    }
}

void Engine::removeNetworkDevice(const std::string & deviceName)
{
    TTValue         v, none;
    TTSymbol        applicationName(deviceName);
    TTSymbol        protocolName;
    TTObjectBasePtr anApplication = getApplication(applicationName);
    
    // if the application exists
    if (anApplication) {
        
        // get the protocols of the application
        v = getApplicationProtocols(applicationName);
        protocolName = v[0]; // we register application to 1 protocol only
        
        // stop the protocol for this application
        getProtocol(protocolName)->sendMessage(TTSymbol("Stop"));
        
        // unregister the application to the protocol
        getProtocol(protocolName)->sendMessage(TTSymbol("unregisterApplication"), applicationName, none);
        
        // delete the application
        TTObjectBaseRelease(TTObjectBaseHandle(&anApplication));
    }
}

void Engine::sendNetworkMessage(const std::string & stringToSend)
{    
    TTValue out, data, v = TTString(stringToSend);
    v.fromString();
    
    TTSymbol aSymbol = v[0];
    TTAddress anAddress = toTTAddress(aSymbol.string().data());
    data.copyFrom(v, 1);
    
    m_sender->setAttributeValue(kTTSym_address, anAddress);
    m_sender->sendMessage(kTTSym_Send, data, out);
}

void Engine::getProtocolNames(std::vector<std::string>& allProtocolNames)
{
    TTValue     protocolNames;
    TTSymbol    name;
    
    // get all protocol names
    TTModularApplications->getAttributeValue(TTSymbol("protocolNames"), protocolNames);
    
    for (TTUInt8 i = 0; i < protocolNames.size(); i++) {
        
        name = protocolNames[i];
        
        allProtocolNames.push_back(name.c_str());
    }
}

void Engine::getNetworkDevicesName(std::vector<std::string>& allDeviceNames)
{
    TTValue     applicationNames;
    TTSymbol    name;
    
    // get all application name
    TTModularApplications->getAttributeValue(TTSymbol("applicationNames"), applicationNames);
    
    for (TTUInt8 i = 0; i < applicationNames.size(); i++) {
        
        // don't return the local application
        name = applicationNames[i];
        
        if (name == getLocalApplicationName)
            continue;
        
        allDeviceNames.push_back(name.c_str());
    }
}

bool Engine::isNetworkDeviceRequestable(const std::string deviceName)
{
    
    TTSymbol    name = TTSymbol(deviceName);
    TTBoolean   discover = NO;
    
    // get all protocol names used by this application
    TTValue protocolNames = getApplicationProtocols(name);
    
    // if there is at least one protocol,
    if (protocolNames.size()) {
        
        // look if it provides namespace exploration
        name = protocolNames[0];
        discover = getProtocol(name)->mDiscover;
    }
    
    return discover;
}

std::vector<std::string> Engine::requestNetworkSnapShot(const std::string & address)
{
    vector<string>      snapshot;
    TTAddress           anAddress = toTTAddress(address);
    TTNodeDirectoryPtr  aDirectory;
    TTNodePtr           aNode;
    TTObjectBasePtr     anObject;
    TTString            s;
    TTValue             v;
    
    // get the application directory
    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    
    if (aDirectory) {
    
        // get the node
        if (!aDirectory->getTTNode(anAddress, &aNode)) {
            
            // get object attributes
            anObject = aNode->getObject();
            
            if (anObject) {
                
                // in case of proxy data or mirror object
                if (anObject->getName() == TTSymbol("Data") ||
                    (anObject->getName() == kTTSym_Mirror && TTMirrorPtr(anObject)->getName() == TTSymbol("Data")))
                {
                    // get the value attribute
                    anObject->getAttributeValue(TTSymbol("value"), v);
                    v.toString();
                    s = TTString(v[0]);
                    
                    // append address value to the snapshot
                    snapshot.push_back(address + " " + s.data());
                }
            }
        }
    }
    
    return snapshot;
}

int
Engine::requestObjectAttributeValue(const std::string & address, const std::string & attribute, vector<string>& value)
{
    TTAddress           anAddress = toTTAddress(address);
    TTNodeDirectoryPtr  aDirectory;
    TTNodePtr           aNode;
    TTMirrorPtr         aMirror;
    TTString            s;
    TTValue             v;

    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    value.clear();

    if (!aDirectory)
        return 1;

    if (!aDirectory->getTTNode(anAddress, &aNode)) {

        // get object attributes
        aMirror = TTMirrorPtr(aNode->getObject());

        if (aMirror) {
            if(!aMirror->getAttributeValue(TTSymbol(attribute), v)){               
                v.toString();
                s = TTString(v[0]);
                value.push_back(s.c_str());
                return 1;
            }
        }
    }
    return 0;
}

int Engine::setObjectAttributeValue(const std::string & address, const std::string & attribute, std::string & value)
{
    TTAddress           anAddress = toTTAddress(address);
    TTNodeDirectoryPtr  aDirectory;
    TTNodePtr           aNode;
    TTMirrorPtr         aMirror;
    TTValue             v;
    
    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    value.clear();
    
    if (!aDirectory)
        return 1;
    
    if (!aDirectory->getTTNode(anAddress, &aNode)) {
        
        // set object attributes
        aMirror = TTMirrorPtr(aNode->getObject());
        
        if (aMirror) {
            
            v = TTString(value);
            v.fromString();
      
            if(!aMirror->setAttributeValue(TTSymbol(attribute), v))
                return 1;
        }
    }
    return 0;
}

int
Engine::requestObjectType(const std::string & address, std::string & nodeType){
    TTNodeDirectoryPtr  aDirectory;
    TTAddress           anAddress = toTTAddress(address);
    TTSymbol            type;
    TTObjectBasePtr     anObject;
    TTMirrorPtr         aMirror;
    TTNodePtr           aNode;

    nodeType = "none";
    aDirectory = getApplicationDirectory(anAddress.getDirectory());

    if (!aDirectory)
        return 0;

    if (!aDirectory->getTTNode(anAddress, &aNode)) {

        anObject = aNode->getObject();
        
        if (anObject) {
            
            if (anObject->getName() == kTTSym_Mirror)
                type = TTMirrorPtr(anObject)->getName();
            else
                type = anObject->getName();
            
            if (type != kTTSymEmpty) {
                
                nodeType = type.c_str();
                return 1;
            }
        }
    }
    return 0;
}

int
Engine::requestObjectPriority(const std::string &address, unsigned int &priority){
    TTNodeDirectoryPtr  aDirectory;
    TTAddress           anAddress = toTTAddress(address);
    TTObjectBasePtr     anObject;
    TTNodePtr           aNode;
    TTValue             v;

    aDirectory = getApplicationDirectory(anAddress.getDirectory());

    if (!aDirectory)
        return 0;

    if (!aDirectory->getTTNode(anAddress, &aNode)) {

        anObject = aNode->getObject();

        if (anObject) {

            if (!anObject->getAttributeValue(kTTSym_priority, v))
                priority = v[0];
            else
                return 1;
        }
    }
    return 0;
}

int
Engine::requestObjectChildren(const std::string & address, vector<string>& children)
{
    TTNodeDirectoryPtr  aDirectory;
    TTAddress           anAddress = toTTAddress(address);
    TTNodePtr           aNode, childNode;
    TTList              nodeList;
    TTString            s;

    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    children.clear();

    if (!aDirectory)
        return 0;

    if (!aDirectory->getTTNode(anAddress, &aNode)) {

        aNode->getChildren(S_WILDCARD, S_WILDCARD, nodeList);
        nodeList.sort(&TTModularCompareNodePriorityThenNameThenInstance);

        for (nodeList.begin(); nodeList.end(); nodeList.next()) {

            childNode = TTNodePtr(TTPtr(nodeList.current()[0]));

            s = childNode->getName().string();
            if (childNode->getInstance() != kTTSymEmpty) {
                s += ".";
                s += childNode->getInstance().string();
            }
            children.push_back(s.c_str());
        }
        return 1;
    }
    return 0;
}

bool Engine::rebuildNetworkNamespace(const string &deviceName, const string &address)
{
    TTValue         v, none;
    TTSymbol        applicationName(deviceName);
    TTSymbol        protocolName;
    TTObjectBasePtr anApplication = getApplication(applicationName);
    
    // if the application exists
    if (anApplication) {
        
        // get the protocols of the application
        v = getApplicationProtocols(applicationName);
        protocolName = v[0]; // we register application to 1 protocol only
        
        // Minuit case : use discovery mechanism
        if (protocolName == TTSymbol("Minuit")) {
            
            anApplication->sendMessage("DirectoryBuild");
            return 0;
        }
        // OSC case : reload the namespace from the last project file if exist
        else if (protocolName == TTSymbol("OSC")) {
            
            if (m_namespaceFilesPath.find(deviceName) == m_namespaceFilesPath.end())
                return 1;
            
            TTSymbol namespaceFilePath = TTSymbol(m_namespaceFilesPath[deviceName]);
            
            // Create a TTXmlHandler
            TTObject aXmlHandler(kTTSym_XmlHandler);
            
            // Read the file to setup TTModularApplications
            v = TTObjectBasePtr(anApplication);
            aXmlHandler.set(kTTSym_object, v);
            aXmlHandler.send(kTTSym_Read, namespaceFilePath, none);
            
            return 0;
        }
    }
    
    return 1;
}

bool Engine::loadNetworkNamespace(const string &deviceName, const string &filepath)
{
    // Create a TTXmlHandler
    TTObject aXmlHandler(kTTSym_XmlHandler);
    
    // Read the file to setup an application
    TTValue none, v = TTObjectBasePtr(getApplication(TTSymbol(deviceName)));
    aXmlHandler.set(kTTSym_object, v);
    
    TTErr err = aXmlHandler.send(kTTSym_Read, TTSymbol(filepath), none);
    
    if (! err) {
    
        // Init the application
        getApplication(TTSymbol(deviceName))->sendMessage(TTSymbol("Init"));
        
        // store the namespace file for this device
        m_namespaceFilesPath[deviceName] = filepath;
    }

    return err != kTTErrNone;
}

bool Engine::getDeviceIntegerParameter(const string device, const string protocol, const string parameter, unsigned int &integer){
    TTSymbol        applicationName;
    TTErr           err;
    TTValue         p, v;
    TTHashPtr       hashParameters;

    applicationName = TTSymbol(device);

    // get parameter's table
    v = TTSymbol(applicationName);
    err = getProtocol(TTSymbol(protocol))->getAttributeValue(TTSymbol("applicationParameters"), v);

    if (!err) {
        hashParameters = TTHashPtr((TTPtr)v[0]);
        hashParameters->lookup(TTSymbol(parameter),p);
        integer = TTUInt32(p[0]);
        return 0;
    }
    return 1;
}

bool Engine::getDeviceIntegerVectorParameter(const string device, const string protocol, const string parameter, vector<int>& integerVect){
    TTSymbol        applicationName;
    TTErr           err;
    TTValue         p, v;
    TTHashPtr       hashParameters;

    applicationName = TTSymbol(device);

    // get parameter's table
    v = TTSymbol(applicationName);
    err = getProtocol(TTSymbol(protocol))->getAttributeValue(TTSymbol("applicationParameters"), v);

    if (!err) {
        hashParameters = TTHashPtr((TTPtr)v[0]);
        hashParameters->lookup(TTSymbol(parameter),p);

        for (TTUInt32 i = 0 ; i < p.size() ; i++)
            integerVect.push_back(TTUInt16(p[i]));

        return 0;
    }
    return 1;
}

bool Engine::getDeviceStringParameter(const string device, const string protocol, const string parameter, string &string){

    TTSymbol        applicationName, s;
    TTErr           err;
    TTValue         p, v;
    TTHashPtr       hashParameters;

    applicationName = TTSymbol(device);

    // get parameter's table
    v = TTSymbol(applicationName);
    err = getProtocol(TTSymbol(protocol))->getAttributeValue(TTSymbol("applicationParameters"), v);

    if (!err) {
        hashParameters = TTHashPtr((TTPtr)v[0]);
        hashParameters->lookup(TTSymbol(parameter),p);
        s = p[0];
        string = s.c_str();
        return 0;
    }
    return 1;
}

bool
Engine::getDeviceProtocol(std::string deviceName, std::string &protocol)
{
    TTValue         v;
    TTSymbol        applicationName(deviceName);
    TTSymbol        protocolName;
    TTObjectBasePtr anApplication = getApplication(applicationName);

    // if the application exists
    if (anApplication) {
        // get the protocols of the application
        v = getApplicationProtocols(applicationName);
        protocolName = v[0]; // we register application to 1 protocol only
        protocol = protocolName.c_str();
        return 0;
    }
    return 1;
}

bool
Engine::setDeviceName(string deviceName, string newName)
{
    string          protocol,
                    localHost;
    unsigned int    port;

    //get protocol name
    if(getDeviceProtocol(deviceName,protocol) != 0)
        return 1;

    //get port
    if(getDeviceIntegerParameter(deviceName,protocol,"port",port) != 0)
        return 1;

    //get ip
    if(getDeviceStringParameter(deviceName,protocol,"ip",localHost) != 0)
        return 1;

    addNetworkDevice(newName,protocol,localHost,port);
    removeNetworkDevice(deviceName);

    return 0;
}

bool
Engine::setDevicePort(string deviceName, int destinationPort, int receptionPort)
{
    TTValue         v, portValue;
    TTSymbol        applicationName(deviceName);
    TTHashPtr       hashParameters;
    TTErr           err;
    std::string     protocol;

    v = TTSymbol(applicationName);

    if (getDeviceProtocol(deviceName,protocol) != 0)
        return 1;

    err = getProtocol(TTSymbol(protocol))->getAttributeValue(TTSymbol("applicationParameters"), v);

    if (!err) {
        
        // stop the protocol
        getProtocol(TTSymbol(protocol))->sendMessage(TTSymbol("Stop"));
        
        hashParameters = TTHashPtr((TTPtr)v[0]);

        hashParameters->remove(TTSymbol("port"));
        
        portValue = destinationPort;
        if (receptionPort != 0)
            portValue.append(receptionPort);
        
        hashParameters->append(TTSymbol("port"), portValue);

        v = TTSymbol(applicationName);
        v.append((TTPtr)hashParameters);
        getProtocol(TTSymbol(protocol))->setAttributeValue(TTSymbol("applicationParameters"), v);
        
        // run the protocol
        getProtocol(TTSymbol(protocol))->sendMessage(TTSymbol("Run"));

        return 0;
    }

    return 1;
}

bool
Engine::setDeviceLocalHost(string deviceName, string localHost)
{
    TTValue         v;
    TTSymbol        applicationName(deviceName);
    TTHashPtr       hashParameters;
    TTErr           err;
    std::string     protocol;

    v = TTSymbol(applicationName);

    if(getDeviceProtocol(deviceName,protocol) != 0)
        return 1;

    err = getProtocol(TTSymbol(protocol))->getAttributeValue(TTSymbol("applicationParameters"), v);

    if (!err) {
        
        // stop the protocol
        getProtocol(TTSymbol(protocol))->sendMessage(TTSymbol("Stop"));
        
        hashParameters = TTHashPtr((TTPtr)v[0]);

        hashParameters->remove(TTSymbol("ip"));
        hashParameters->append(TTSymbol("ip"), TTSymbol(localHost));

        v = TTSymbol(applicationName);
        v.append((TTPtr)hashParameters);
        getProtocol(TTSymbol(protocol))->setAttributeValue(TTSymbol("applicationParameters"), v);
        
        // run the protocol
        getProtocol(TTSymbol(protocol))->sendMessage(TTSymbol("Run"));

        return 0;
    }

    return 1;
}

bool
Engine::setDeviceProtocol(string deviceName, string protocol)
{
    string              oldProtocol,
                        localHost;
    unsigned int        port;

    //get protocol name
    if(getDeviceProtocol(deviceName,oldProtocol) != 0)
        return 1;

    //get ip
    if(getDeviceStringParameter(deviceName,oldProtocol,"ip",localHost) != 0)
        return 1;

    //get port
    if(getDeviceIntegerParameter(deviceName,oldProtocol,"port",port) != 0)
        return 1;

    removeNetworkDevice(deviceName);
    addNetworkDevice(deviceName,protocol,localHost,port);    

    return 0;
}

bool Engine::setDeviceLearn(std::string deviceName, bool newLearn)
{
    TTSymbol applicationName(deviceName);
    
    TTErr err = getApplication(applicationName)->setAttributeValue("learn", newLearn);
    
    // enable namespace observation
    if (newLearn && !m_namespaceObserver) {
        
        TTValue     none;
        TTValuePtr  baton;
        
        // create a TTCallback to observe when a node is created (using NamespaceCallback)
        TTObjectBaseInstantiate("callback", &m_namespaceObserver, none);
        
        baton = new TTValue(TTPtr(this)); // baton will be deleted during the callback destruction
        baton->append(applicationName);
        
        m_namespaceObserver->setAttributeValue(kTTSym_baton, TTPtr(baton));
        m_namespaceObserver->setAttributeValue(kTTSym_function, TTPtr(&NamespaceCallback));
        //m_namespaceObserver->setAttributeValue(kTTSym_notification, ???);
        
        getApplicationDirectory(applicationName)->addObserverForNotifications(kTTAdrsRoot, TTCallbackPtr(m_namespaceObserver));
    }
    // disable namespace observation
    else if (!newLearn && m_namespaceObserver) {
        
        getApplicationDirectory(applicationName)->removeObserverForNotifications(kTTAdrsRoot, TTCallbackPtr(m_namespaceObserver));
        
        TTObjectBaseRelease(&m_namespaceObserver);
        m_namespaceObserver = NULL;
    }
    
    return err != kTTErrNone;
}

bool Engine::getDeviceLearn(std::string deviceName)
{
    TTSymbol    applicationName(deviceName);
    TTValue     v;
    
    getApplication(applicationName)->getAttributeValue("learn", v);
    
    return TTBoolean(v[0]);
}

int Engine::requestNetworkNamespace(const std::string & address, std::string & nodeType, vector<string>& nodes, vector<string>& leaves, vector<string>& attributs, vector<string>& attributsValue)
{
    TTAddress           anAddress = toTTAddress(address);
    TTSymbol            type, service;
    TTNodeDirectoryPtr  aDirectory;
    TTNodePtr           aNode, childNode;
    TTMirrorPtr         aMirror;
    TTList              nodeList;
    TTString            s;
    TTValue             v;
    
    // get the application directory
    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    
    if (!aDirectory)
        return 0;
    
    // explore the directory at this address
    // notice the tree is already built (see in initModular)
    if (!aDirectory->getTTNode(anAddress, &aNode)) {
        
        // get object attributes
        aMirror = TTMirrorPtr(aNode->getObject());
        if (aMirror) {
            
            type = aMirror->getName();
            
            if (type != kTTSymEmpty)
                nodeType = type.c_str();
            else
                nodeType = "none";
            
            if (type == TTSymbol("Data")) {
                
                // append a service attribute
                attributs.push_back("service");
                
                // get the value of the service attribute
                if (!aMirror->getAttributeValue(TTSymbol("service"), v))
                    service = v[0];
                else
                    service = TTSymbol("error");
                
                v.toString();
                s = TTString(v[0]);
                attributsValue.push_back(s.c_str());
                
                // for parameter data : ask the value
                if (service == kTTSym_parameter) {
                
                    // append the value attribute
                    attributs.push_back("value");
                
                    // get the value attribute
                    aMirror->getAttributeValue(TTSymbol("value"), v);
                    v.toString();
                    s = TTString(v[0]);
                    attributsValue.push_back(s.c_str());
                }
                
                // for any data : ask the rangeBounds
                // append the value attribute
                attributs.push_back("rangeBounds");
                
                // get the value of the rangeBounds attribute
                aMirror->getAttributeValue(TTSymbol("rangeBounds"), v);
                    
                v.toString();
                s = TTString(v[0]);
                attributsValue.push_back(s.c_str());
            }
            else if (type == TTSymbol("Container")) {
                
                // append a service attribute
                attributs.push_back("service");
                
                // get the value of the service attribute
                if (!aMirror->getAttributeValue(TTSymbol("service"), v))
                    service = v[0];
                else
                    service = TTSymbol("error");
                
                v.toString();
                s = TTString(v[0]);
                attributsValue.push_back(s.c_str());
            }
        }
        
        // TODO : get attributes value
        
        // get children
        aNode->getChildren(S_WILDCARD, S_WILDCARD, nodeList);
        
        // sort children
        nodeList.sort(&TTModularCompareNodePriorityThenNameThenInstance);

        // sort children in leaves and nodes
        for (nodeList.begin(); nodeList.end(); nodeList.next()) {
            
            childNode = TTNodePtr(TTPtr(nodeList.current()[0]));
            
            // prepare name.instance to store
            s = childNode->getName().string();
            if (childNode->getInstance() != kTTSymEmpty) {
                s += ".";
                s += childNode->getInstance().string();
            }
            
            // depending on the mirror object type
            aMirror = TTMirrorPtr(childNode->getObject());
            if (aMirror) {
                
                type = aMirror->getName();
                
                if (type == TTSymbol("Data"))
                    leaves.push_back(s.c_str());
                else
                    nodes.push_back(s.c_str());
                    
            }
            else
                nodes.push_back(s.c_str());
        }
        
        return 1;
    }
    
    return 0;
}

int Engine::appendToNetWorkNamespace(const std::string & address, const std::string & service, const std::string & type, const std::string & priority, const std::string & description, const std::string & range, const std::string & clipmode, const std::string & tags)
{
    TTAddress           anAddress = toTTAddress(address);
    TTNodeDirectoryPtr  aDirectory;
    TTObjectBasePtr     anObject;
    TTString            s;
    TTValue             v, out;
    
    // get the application directory
    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    
    if (!aDirectory)
        return 0;
    
    // don't register anything into the local directory
    if (aDirectory == getLocalDirectory)
        return 0;
    
    // create a proxy data
    v = TTValue(anAddress, TTSymbol(service.data()));
    if (!getApplication(anAddress.getDirectory())->sendMessage("ProxyDataInstantiate", v, out)) {
        
        anObject = out[0];
        
        anObject->setAttributeValue("type", TTSymbol(type.data()));
        
        v = TTString(priority);
        v.fromString();
        anObject->setAttributeValue("priority", v);
        
        anObject->setAttributeValue("description", TTSymbol(description.data()));
        
        v = TTString(range);
        v.fromString();
        anObject->setAttributeValue("rangeBounds", v);
        
        anObject->setAttributeValue("rangeClipmode", TTSymbol(clipmode.data()));
        
        v = TTString(tags);
        v.fromString();
        anObject->setAttributeValue("tag", v);
        
        return 1;
    }
    
    return 0;
}

int Engine::removeFromNetWorkNamespace(const std::string & address)
{
    TTAddress           anAddress = toTTAddress(address);
    TTNodeDirectoryPtr  aDirectory;
    TTNodePtr           aNode;
    TTObjectBasePtr     aMirror;
    
    // get the application directory
    aDirectory = getApplicationDirectory(anAddress.getDirectory());
    
    if (!aDirectory)
        return 0;
    
    // explore the directory at this address
    // notice the tree is already built (see in initModular)
    if (!aDirectory->getTTNode(anAddress, &aNode)) {
        
        // get object attributes
        aMirror = TTMirrorPtr(aNode->getObject());
        if (aMirror) {
            
            aDirectory->TTNodeRemove(anAddress);
            
            TTObjectBaseRelease(&aMirror);
            
            return 1;
        }
    }
    
    return 0;
}


// LOAD AND STORE

void Engine::store(std::string filepath)
{
    TTValue v, none;
    
    m_lastProjectFilePath = TTSymbol(filepath);
    
    // Create a TTXmlHandler
    TTObject aXmlHandler(kTTSym_XmlHandler);
    
    // Pass the application manager and the main scenario object
    v = TTObjectBasePtr(TTModularApplications);
    v.append(m_mainScenario);
    aXmlHandler.set(kTTSym_object, v);
    
    // Write
    aXmlHandler.send(kTTSym_Write, m_lastProjectFilePath, none);
}

void Engine::load(std::string filepath)
{
    TTValue v, none;
    
    // Check that all Engine caches have been properly cleared before
    if (m_timeProcessMap.size() > 1)
        TTLogMessage("Engine::load : m_timeProcessMap not empty before the loading\n");
    
    if (!m_intervalMap.empty())
        TTLogMessage("Engine::load : m_intervalMap not empty before the loading\n");
    
    if (!m_conditionedProcessMap.empty())
        TTLogMessage("Engine::load : m_conditionedProcessMap not empty before the loading\n");
    
    if (!m_timeConditionMap.empty())
        TTLogMessage("Engine::load : m_timeConditionMap not empty before the loading\n");
    
    m_lastProjectFilePath = TTSymbol(filepath);
    
    // Create a TTXmlHandler
    TTObject aXmlHandler(kTTSym_XmlHandler);
    
    // Read the file to setup TTModularApplications
    v = TTObjectBasePtr(TTModularApplications);
    aXmlHandler.set(kTTSym_object, v);
    aXmlHandler.send(kTTSym_Read, m_lastProjectFilePath, none);
    
    // Read the file to setup m_mainScenario
    v = TTObjectBasePtr(m_mainScenario);
    aXmlHandler.set(kTTSym_object, v);
    aXmlHandler.send(kTTSym_Read, m_lastProjectFilePath, none);
    
    // Rebuild all the EngineCacheMaps from the main scenario content
    buildEngineCaches(m_mainScenario, kTTAdrsRoot);
}

void Engine::buildEngineCaches(TTTimeProcessPtr scenario, TTAddress& scenarioAddress)
{
    TTValue             v, objects, none;
    TTTimeProcessPtr    timeProcess;
    TTTimeEventPtr      timeEvent;
    TTTimeConditionPtr  timeCondition;
    TTSymbol            name;
    TimeProcessId       timeProcessId;
    IntervalId          relationId;
    ConditionedProcessId triggerId;
    TimeConditionId     timeConditionId;
    
    // temporary map from TTTimeConditionPtr to TimeConditionId
    std::map<TTTimeConditionPtr, TimeConditionId> TTCondToID;

    // get all TTTimeConditions
    scenario->getAttributeValue(TTSymbol("timeConditions"), objects);

    // for all time conditions
    for (TTUInt32 i = 0 ; i < objects.size() ; ++i) {

        timeCondition = TTTimeConditionPtr(TTObjectBasePtr(objects[i]));

        // check if it's a condition for i-score (2-plus events)
        timeCondition->getAttributeValue(TTSymbol("events"), v);
        if (v.size() >= 2) {

            // get a unique ID for the condition
            timeConditionId = m_nextConditionedProcessId++;

            // cache it
            cacheTimeCondition(timeConditionId, timeCondition);

            // fill the temporary map
            TTCondToID[timeCondition] = timeConditionId;
        }
    }
    
    // get all TTTimeProcesses
    scenario->getAttributeValue(TTSymbol("timeProcesses"), objects);
    
    // for all time process
    for (TTUInt32 i = 0; i < objects.size(); i++) {
        
        timeProcess = TTTimeProcessPtr(TTObjectBasePtr(objects[i]));
        
        // for each Automation process
        if (timeProcess->getName() == TTSymbol("Automation")) {
            
            timeProcess->getAttributeValue(kTTSym_name, v);
            name = v[0];
            
            // Cache it and get an unique id for this process
            TTAddress address = scenarioAddress.appendAddress(TTAddress(name));
            timeProcessId = cacheTimeProcess(timeProcess, address);
            
            // if the Start event of the Automation process is conditioned
            timeProcess->getAttributeValue(TTSymbol("startEvent"), v);
            timeEvent = TTTimeEventPtr(TTObjectBasePtr(v[0]));
            
            timeEvent->getAttributeValue(TTSymbol("condition"), v);
            timeCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));
            
            if (timeCondition != NULL) {
            
                // We cache the time process and the event index instead of the event itself
                triggerId = cacheConditionedProcess(timeProcessId, BEGIN_CONTROL_POINT_INDEX);
            
                // We cache the TTTimeCondition
                cacheTimeCondition(triggerId, timeCondition);

                // if it is a condition for i-score
                std::map<TTTimeConditionPtr, TimeConditionId>::iterator it = TTCondToID.find(timeCondition);
                if (it != TTCondToID.end()) {

                    // add it in the conditions map
                    m_conditionsMap[it->second].push_back(triggerId);
                }
            }
            
            // if the End event of the Automation process is conditioned
            timeProcess->getAttributeValue(TTSymbol("endEvent"), v);
            timeEvent = TTTimeEventPtr(TTObjectBasePtr(v[0]));
            
            timeEvent->getAttributeValue(TTSymbol("condition"), v);
            timeCondition = TTTimeConditionPtr(TTObjectBasePtr(v[0]));
            
            if (timeCondition != NULL) {
                
                // We cache the time process and the event index instead of the event itself
                triggerId = cacheConditionedProcess(timeProcessId, END_CONTROL_POINT_INDEX);
            
                // We cache the TTTimeCondition
                cacheTimeCondition(triggerId, timeCondition);
            }
        }
        
        // for each Interval process
        else if (timeProcess->getName() == TTSymbol("Interval")) {
            
            // Cache it and get an unique id for this process
            relationId = cacheInterval(timeProcess);
        }
        
        // for each Scenario process
        else if (timeProcess->getName() == TTSymbol("Scenario")) {
            
            // get end and start events
            TTTimeEventPtr startSubScenario, endSubScenario;
            
            timeProcess->getAttributeValue(TTSymbol("startEvent"), v);
            startSubScenario = TTTimeEventPtr(TTObjectBasePtr(v[0]));
            
            timeProcess->getAttributeValue(TTSymbol("endEvent"), v);
            endSubScenario = TTTimeEventPtr(TTObjectBasePtr(v[0]));

            // retreive the time process with the same end and start events
            EngineCacheMapIterator it;
            TTAddress address;
            
            for (it = m_timeProcessMap.begin(); it != m_timeProcessMap.end(); ++it) {
                
                TTTimeEventPtr  start, end;
                
                it->second->object->getAttributeValue(TTSymbol("startEvent"), v);
                start = TTTimeEventPtr(TTObjectBasePtr(v[0]));
                
                it->second->object->getAttributeValue(TTSymbol("endEvent"), v);
                end = TTTimeEventPtr(TTObjectBasePtr(v[0]));
                
                // set the scenario as the subScenario related to this time process
                if (start == startSubScenario && end == endSubScenario) {
                    
                    it->second->subScenario = TTObjectBasePtr(timeProcess);
                    address = it->second->address;
                    break;
                }
            }
            
            // Rebuild all the EngineCacheMaps from the sub scenario content
            buildEngineCaches(timeProcess, address);
        }
    }
}

// NETWORK
void Engine::print() {
#ifdef TODO_ENGINE
	m_executionMachine->getPetriNet()->print();
#endif
}

void Engine::printExecutionInLinuxConsole()
{
#ifdef TODO_ENGINE    
	std::vector<TimeProcessId> boxesId;
    
	getBoxesId(boxesId);
    
	bool mustDisplay = true;
    
	while(isPlaying()){
        
		if ((getCurrentExecutionTime()/60)%2 == 0) {
			if (mustDisplay) {
				std::system("clear");
				for (unsigned int i = 0; i < boxesId.size(); ++i) {
					unsigned int processPercent;
                    
					processPercent = getProcessPosition(boxesId[i]);
                    
					if ((processPercent > 0) && (processPercent < 99)) {
						std::cout << "[*";
                        
						for (unsigned j = 10; j <= processPercent; j += 10) {
							std::cout << "*";
						}
                        
						for (unsigned j = processPercent + 1; j <= 90; j += 10) {
							std::cout << ".";
						}
                        
						std::cout << "] -> (" << i << ") (" << processPercent << ")" << std::endl;
					}
				}
				mustDisplay = false;
			}
		} else {
			mustDisplay = true;
		}
		usleep(50);
	}
#endif
}

#if 0
#pragma mark -
#pragma mark Callback Methods
#endif

void TimeEventStatusAttributeCallback(const TTValue& baton, const TTValue& value)
{
    TTValuePtr              b;
    EnginePtr               engine;
    ConditionedProcessId    triggerId;
    TTObjectBasePtr         event;
    TTValue                 v;
    TTSymbol                status;
	
	// unpack baton (engine, triggerId)
	b = (TTValuePtr)baton;
	engine = EnginePtr((TTPtr)(*b)[0]);
    triggerId = ConditionedProcessId((*b)[1]);
	
	// Unpack data (event)
	event = value[0];
    
    // get status
    event->getAttributeValue(kTTSym_status, v);
    status = v[0];
    
	if (engine->m_TimeEventStatusAttributeCallback != NULL) {
        
        if (status == kTTSym_eventWaiting) {
            engine->m_TimeEventStatusAttributeCallback(triggerId, false);
        }
        else if (status == kTTSym_eventPending) {
            engine->m_TimeEventStatusAttributeCallback(triggerId, true);
        }
        else if (status == kTTSym_eventHappened) {
            engine->m_TimeEventStatusAttributeCallback(triggerId, false);
        }
        else if (status == kTTSym_eventDisposed) {
            engine->m_TimeEventStatusAttributeCallback(triggerId, false);
        }
        
        iscoreEngineDebug {
            
            if (status == kTTSym_eventWaiting) {
                TTLogMessage("TriggerPoint %ld is waiting\n", triggerId);
            }
            else if (status == kTTSym_eventPending) {
                TTLogMessage("TriggerPoint %ld is pending\n", triggerId);
            }
            else if (status == kTTSym_eventHappened) {
                TTLogMessage("TriggerPoint %ld happened\n", triggerId);
            }
            else if (status == kTTSym_eventDisposed) {
                TTLogMessage("TriggerPoint %ld is disposed\n", triggerId);
            }
        }
    }
}

void TimeProcessStartCallback(const TTValue& baton, const TTValue& value)
{
    TTValuePtr      b;
    EnginePtr       engine;
    TimeProcessId   boxId;
	
	// unpack baton (engine, boxId)
	b = (TTValuePtr)baton;
	engine = EnginePtr((TTPtr)(*b)[0]);
    boxId = TTUInt32((*b)[1]);
    
    iscoreEngineDebug 
        TTLogMessage("TimeProcess %ld starts at %ld ms\n", boxId, engine->getCurrentExecutionDate());
        
    if (engine->m_TimeProcessSchedulerRunningAttributeCallback != NULL)
        engine->m_TimeProcessSchedulerRunningAttributeCallback(boxId, YES);

}

void TimeProcessEndCallback(const TTValue& baton, const TTValue& value)
{
    TTValuePtr      b;
    EnginePtr       engine;
    TimeProcessId   boxId;
	
	// unpack baton (engine, boxId)
	b = (TTValuePtr)baton;
	engine = EnginePtr((TTPtr)(*b)[0]);
    boxId = TTUInt32((*b)[1]);
    
    iscoreEngineDebug
        TTLogMessage("TimeProcess %ld ends at %ld ms\n", boxId, engine->getCurrentExecutionDate());
    
    // update all process running state too
    if (engine->m_TimeProcessSchedulerRunningAttributeCallback != NULL)
        engine->m_TimeProcessSchedulerRunningAttributeCallback(boxId, NO);
    
}

void NamespaceCallback(const TTValue& baton, const TTValue& value)
{
    TTValuePtr  b;
    EnginePtr   engine;
    TTSymbol    applicationName;
	TTUInt8     flag;
	
	// unpack baton (engine, applicationName)
	b = (TTValuePtr)baton;
	engine = EnginePtr((TTPtr)(*b)[0]);
    applicationName = (*b)[1];
    
    // Unpack value (anAddress, aNode, flag, anObserver)
	flag = value[2];
    
    if (flag == kAddressCreated)
        engine->m_NetworkDeviceNamespaceCallback(applicationName);
}

TTAddress Engine::toTTAddress(string networktreeAddress)
{
    TTSymbol            temp(networktreeAddress);
    TTAddress           address(temp);
    TTAddress           applicationName, anAddress;
    TTString            s;
    
    // split the address to get application name and then an address
    address.splitAt(0, applicationName, anAddress);
    
    // edit applicationName:/anAddress
    s = applicationName.string();
    s += S_DIRECTORY.string();
    
    if (anAddress != kTTAdrsEmpty)
        s += anAddress.string();
    
    return TTAddress(s);
}

std::string Engine::toNetworkTreeAddress(TTAddress aTTAddress)
{
    std::string s = aTTAddress.getDirectory().string().c_str();
    s += aTTAddress.normalize().string();
    
    return s;
}

#if 0
#pragma mark -
#pragma mark Miscellaneous Methods
#endif

TTBoolean TTModularCompareNodePriorityThenNameThenInstance(TTValue& v1, TTValue& v2)
{
	TTNodePtr	n1, n2;
	TTObjectBasePtr o1, o2;
	TTValue		v;
    TTValue    name1;
    TTValue    name2;
    TTValue    instance1;
    TTValue    instance2;
	TTInt32		p1 = 0;
	TTInt32		p2 = 0;
	
	// get priority of v1
	n1 = TTNodePtr((TTPtr)v1[0]);
	if (n1) {
        
        name1 = n1->getName();
        instance1 = n1->getInstance();
		o1 = n1->getObject();
		
		if (o1)
			if (!o1->getAttributeValue(kTTSym_priority, v))
				p1 = v[0];
	}
	
	// get priority of v2
	n2 = TTNodePtr((TTPtr)v2[0]);
	if (n2) {
        
        name2 = n2->getName();
        instance2 = n2->getInstance();
		o2 = n2->getObject();
		
		if (o2)
			if (!o2->getAttributeValue(kTTSym_priority, v))
				p2 = v[0];
	}
	
	if (p1 == p2) {
        
        if (name1 == name2) {
            
            if (instance1 == instance2)
                return v1 < v2;
            else
                return instance1 < instance2;
        }
        else
            return name1 < name2;
    }
	
	if (p1 == 0) return NO;
	if (p2 == 0) return YES;
	
	return p1 < p2;
}

