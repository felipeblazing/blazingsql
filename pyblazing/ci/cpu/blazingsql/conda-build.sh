#!/bin/bash

echo "CMD: conda build ${CONDA_CH} -c conda-forge -c defaults --python=$PYTHON conda/recipes/blazingsql/"
conda build ${CONDA_CH} -c conda-forge -c defaults --python=$PYTHON conda/recipes/blazingsql/

