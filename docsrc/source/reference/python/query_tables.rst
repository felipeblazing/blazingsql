Query tables
============

Querying Data on a Single GPU
-----------------------------

BlazingSQL supports a singular interface through our Python package. 

BlazingSQL interoperates with the rest of the `RAPIDS.ai <https://rapids.ai>`_ stack, 
and needs to hand off its results to other technologies in the stack.

.. code-block:: python

    from blazingsql import BlazingContext
    # connect to BlazingSQL
    bc = BlazingContext()

    # create table 
    bc.create_table('table_name', '...file_path/table.csv')

    # run query (returns cuDF DataFrame)
    result = bc.sql('select * from table_name')

Now while that might have been easy to follow there is a little bit to unpack, 
so let's go through it section by section.

Import Packages
```````````````

We need to import the ``BlazingContext`` from BlazingSQL because that is how 
we are going to create a connection to a BlazingSQL instance, and where we 
will store all information regarding the state of that BlazingSQL instance. 

Once the ``BlazingContext`` goes out of scope it will be wiped, which means 
the registered filesystems, created tables, etc. will be gone.

.. code-block:: python

    from blazingsql import BlazingContext
    
Initialize Context
``````````````````

Then we need to initialize the ``BlazingContext``.

Since this is a *Single GPU* instance we do not have to fill in any of the arguments.

.. code-block:: python

    bc = BlazingContext()
    
Create a Table
``````````````

Now with our ``BlazingContext``, we can run commands on the BlazingSQL 
instance we are connected to. We will first create a table.

.. code-block:: python
    
    bc.create_table('table_name' '...file_path/table.csv')

Query a Table
`````````````

Now we can run a SQL query on that table!

When we submit a query through the ``.sql()``  method it is asynchronous, 
which means you can launch many SQL queries at once. 

The results from a BlazingSQL instance are a cuDF DataFrame.

.. code-block:: python
    
    result = bc.sql('select * from table_name')

You can now run further commands on that cuDF DataFrame. 

For example:

* :ref:`Create a new BlazingSQL table <query_cudf>`
* `Convert to a Pandas DataFrame <https://docs.rapids.ai/api/cudf/stable/api.html#cudf.core.dataframe.DataFrame.to_pandas>`_
* `Run a cuML Algorithm <https://docs.rapids.ai/api/cuml/stable/>`_
* `Convert to a PyArrow Table <https://docs.rapids.ai/api/cudf/stable/api.html#cudf.core.dataframe.DataFrame.to_arrow>`_

Explain
```````

``BlazingContext`` has an ``.explain()`` method which breaks down a given 
query's algebra plan.

.. code-block:: python

    # define a query
    query = 'SELECT colA, colB FROM table'
    
    # have BlazingContext explain how the query is executed 
    bc.explain(query)

Drop a Table
````````````

To drop a table from ``BlazingContext`` we call ``drop_table`` and pass in 
the name of the table to drop.

.. code-block:: python
    
    bc.drop_table('table_name')
    
Memory Pool
```````````

Upon initialization, ``BlazingContext`` defaults to allocating a 
portion of GPU memory to create tables and execute queries. 
This can greatly improve performance, and can be adjusted with 
the ``pool`` and ``initial_pool_size`` parameters. 

By default ``initial_pool_size=None`` which auto allocates 
about 50% of GPU memory; ``initial_pool_size`` can be set with bytes.

.. code-block:: python

    # turn off pool (pool=True by default)
    bc = BlazingContext(pool=False)

    # allocate 10,000 MiB for pool
    bc = BlazingContext(initial_pool_size=1.0486e+10)

Querying Data on Multiple GPUs
--------------------------------

BlazingSQL can easily distribute up to multiple GPUs and servers by using 
Dask and Dask cuDF.

When BlazingSQL runs in on a single GPU, query results will return as cuDFs. 
When BlazingSQL runs on multiple GPUs, query results will return as Dask cuDFs, 
which are distributed GPU DataFrames.

When BlazingSQL runs on a single node, the entire query execution 
runs on one GPU on one BlazingSQL Engine process. When running on 
distributed mode, it will Dask to run one BlazingSQL Engine process per GPU.

The easiest way to launch a BlazingSQL cluster is if you have multiple 
GPUs on a single machine, in which case you can leverage the 
`Dask CUDA project <https://github.com/rapidsai/dask-cuda>`_ and 
``LocalCUDACluster``, which is included in the BlazingSQL Conda install by default.
Alternatively, for multiple servers you can launch a ``dask-scheduler`` 
and multiple ``dask-workers`` (one for each GPU).

Clusters
````````

Single Node - Multiple GPU with ``LocalCUDACluster``
''''''''''''''''''''''''''''''''''''''''''''''''''''

.. code-block:: python

    from blazingsql import BlazingContext
    from dask_cuda import LocalCUDACluster
    from dask.distributed import Client

    cluster = LocalCUDACluster()
    client = Client(cluster)

    bc = BlazingContext(dask_client = client, network_interface = 'lo')

BlazingSQL is now distributing all query execution across the available 
cluster of GPUs, and can process queries utilizing the combined GPU memory.

If you run queries you must understand you are received Dask cuDFs as the result.

.. code-block:: python

    # create table
    bc.create_table('table_name', '/home/user/table_dir/*')
    
    # define a query 
    query = 'select * from table_name limit 10'
    
    # explain how the query will be executed
    print(bc.explain(query))
    
    # query table
    ddf = bc.sql(query)
    
    # display results 
    print(ddf.head())
    
    # drop table 
    bc.drop_table('table_name')

Multiple Node - Multiple GPU with ``dask-scheduler`` and ``dask-worker``
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

In order to run on Multiple Nodes you will need to manually launch the 
``dask-scheduler`` and the ``dask-workers``.
To launch the ``dask-scheduler`` you only need to run the ``dask-scheduler`` 
command from a terminal. Note that you must be in the conda environment 
on which you installed BlazingSQL. Also, to launch the workers, you 
will need the IP address where ``dask-scheduler`` is running. For these 
instructions we will assume that is ``123.123.123.123``.

.. code-block:: bash

    conda activate bsql
    dask-scheduler

Then you will need to run ``dask-worker`` on each server, once for each 
GPU on that server. For each launch of ``dask-worker`` you will need to 
also set the environment variable ``CUDA_VISIBLE_DEVICES`` so that each 
``dask-worker`` and hence each BlazingSQL Engine only uses that one GPU.

.. code-block:: bash

    # on one terminal
    conda activate bsql
    CUDA_VISIBLE_DEVICES=0 dask-worker 123.123.123.123:8786
    
    # on another terminal
    conda activate bsql
    CUDA_VISIBLE_DEVICES=1 dask-worker 123.123.123.123:8786
    
    # repeat for other GPUs"
  
Once you have all the ``dask-scheduler`` and ``dask-worker`` processes running you 
can start BlazingSQL. You will also need to know what network 
interface your servers are using to communicate with the IP 
address of the ``dask-scheduler``. You can see the different network 
interfaces and what IP addresses they serve with the command ``ifconfig``. 
For the purposes of these instructions we will assume ``eth0``.

.. code-block:: python

    from blazingsql import BlazingContext
    from dask.distributed import Client

    client = Client('123.123.123.123:8786')

    bc = BlazingContext(dask_client = client, network_interface = 'eth0')