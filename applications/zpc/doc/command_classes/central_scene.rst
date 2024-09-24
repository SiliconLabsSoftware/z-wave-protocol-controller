Central Scene
=============

Version supported : 3

.. contents::
   :depth: 2
   :local:
   :backlinks: none


Interview process
*****************

#. :ref:`Central Scene Supported Get <central-scene-supported-get-command>`
#. :ref:`Central Scene Configuration Get <central-scene-configuration-get-command>`

Command Class Commands
**********************

.. _central-scene-supported-get-command:

Central Scene Supported Get 
---------------------------

Trigger on undefined **reported** value of ``NUMBER_OF_SCENES``

Central Scene Supported Report 
------------------------------

Mapping between Report command and attribute store : 

.. list-table:: 
  :header-rows: 1

  * - Z-Wave Command Attribute 
    - Z-Wave Attribute Store
  * - ``Slow Refresh Support``
    - ``CENTRAL_SCENE_SLOW_REFRESH`` 

.. note:: 
  ``Number of Bit Mask bytes``, ``Identical`` used for investigate ``CENTRAL_SCENE_MAX_KEY_ATTRIBUTE`` 

  ``Supported Scenes`` used for calculate ``CENTRAL_SCENE_NUMBER_OF_SCENES``  


Central Scene Notification
--------------------------

Mapping between Notification command and attribute store:

.. list-table:: 
  :header-rows: 1

  * - Z-Wave Command Attribute
    - Z-wave Attribute Store
  * - ``Sequence Number``
    - ``CENTRAL_SCENE_ACTIVE_SCENE_SEQUENCE_NUMBER``
  * - ``Slow Refresh``
    - ``CENTRAL_SCENE_SLOW_REFRESH``

.. note:: 
  ``Key Attribute``, ``Scene Number`` used for calculate ``CENTRAL_SCENE_ACTIVE_SCENE`` 

Central Scene Configuration Set
-------------------------------

Trigger on new **desired** value of ``SLOW_REFRESH``: 

Mapping between attribute store and Set command: 

.. list-table:: 
  :header-rows: 1

  * - Z-wave Attribute Store 
    - Attribute State
    - Z-wave Set Field
  * - ``SLOW_REFRESH``
    - Desired
    - ``Slow Refresh``

.. _central-scene-configuration-get-command:

Central Scene Configuration Get
-------------------------------

Trigger on undefined **reported** value of ``SLOW_REFRESH``:


Central Scene Configuration Report
----------------------------------

Mapping between Report command and attribute store :

.. list-table:: 
  :header-rows: 1

  * - Report Field Command 
    - Z-Wave Attribute Store 
  * - ``Slow Refresh``
    - ``CENTRAL_SCENE_SLOW_REFRESH``


Unify Clusters
**************

UAM files
---------

.. list-table:: 
  :header-rows: 1

  * - UAM File
    - Cluster
    - Comments
  * - ``Scenes.uam``
    - ``Scenes.xml``
    - Mapping between Central Scene command class and Scene cluster

Bindings
--------

.. list-table:: 
  :header-rows: 1

  * - Z-Wave Attribute Store
    - Cluster attribute
    - Comments
  * - ``NUMBER_OF_SCENES``
    - Scene SceneCount
    - Z-Wave -> Cluster (Read only)
  * - ``ACTIVE_SCENE``
    - Scene CurrentScene
    - Z-Wave -> Cluster (Read only)
  * - 0
    - Scene CurrentGroup
    - Z-Wave -> Cluster (Read only). If ``ACTIVE_SCENE`` defined.
  * - 0
    - Scene NameSupport
    - Z-Wave -> Cluster (Read only). If ``ACTIVE_SCENE`` defined.


Command actions
---------------

.. list-table:: 
  :widths: 20 50 30
  :header-rows: 1

  * - Action
    - MQTT Topic
    - Comments
  * - Trigger when user activation
    - ``ucl/by-unid/+/+/Scenes/GeneratedCommands/RecallScene`` 
    - Indicate which Scene was activated
  * - Update Scene state
    - ``ucl/by-unid/+/+/Scenes/Attributes/CurrentScene/Reported`` 
    - Read-only reported value
  * - Update Scene state
    - ``ucl/by-unid/+/+/Scenes/Attributes/CurrentValid/Reported``
    - Read-only reported value