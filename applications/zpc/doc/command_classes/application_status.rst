Application Status
=============

Version supported : 1

.. contents::
   :depth: 2
   :local:
   :backlinks: none


Interview process
*****************


Command Class Commands
**********************

.. _color-switch-supported-get-command:

Application Busy
---------------------------

Mapping between command and attribute store :

.. list-table:: 
  :header-rows: 1

  * - Report Field Command 
    - Z-Wave Attribute Store 
  * - ``Status``
    - ``BUSY_STATUS``
  * - ``Wait Time``
    - ``WAIT_TIME``
Application Rejected Request
------------------------------

Mapping between Report command and attribute store : 

.. list-table:: 
  :header-rows: 1

  * - Z-Wave Command Attribute 
    - Z-Wave Attribute Store
  * - ``Status``
    - ``REJECT_STATUS``  

.. note:: 
    The structure of the attribute store is : 

    .. code:: text
        
        |__ BUSY_STATUS
        |    |__ WAIT_TIME
        |__ REJECT_STATUS

Unify Clusters
**************

UAM files
---------

.. list-table:: 
  :header-rows: 1

  * - UAM File
    - Cluster
    - Comments
  * - ``ApplicationStatus.uam``
    - ``Unify_ApplicationStatus.xml``
    - Mapping between ApplicationStatus command class and ApplicationStatus cluster

Bindings
--------

.. list-table:: 
  :header-rows: 1

  * - Z-Wave Attribute Store
    - Cluster attribute
    - Comments
  * - ``BUSY_STATUS``
    - ApplicationStatus BusyStatus
    - Z-Wave <-> Cluster (Read only)
  * - ``WAIT_TIME``
    - ApplicationStatus WaitTime
    - Z-Wave <-> Cluster (Read only)
  * - ``REJECT_STATUS``
    - ApplicationStatus RejectStatus
    - Z-Wave <-> Cluster (Read only)


Command actions
---------------

.. list-table:: 
  :widths: 20 50 30
  :header-rows: 1

  * - Action
    - MQTT Topic
    - Comments
  * - Report BusyStatus
    - ``ucl/by-unid/<UNID>/ep0/ApplicationStatus/Attributes/BusyStatus {"value" : "Try again later"}`` 
    - ``BusyStatus`` value: ``Try again later`` or ``Try again in Wait Time seconds`` or ``Request queued, executed later``
  * - Report WaitTime 
    - ``ucl/by-unid/<UNID>/ep0/ApplicationStatus/Attributes/WaitTime {"value" : 10}`` 
    - Number of second
  * - Report RejectStatus
    - ``ucl/by-unid/<UNID>/ep0/ApplicationStatus/Attributes/RejectStatus {"value" : false}`` 
    - ``false`` indicate that the received (supported) command has been rejected by the application at the receiving node 