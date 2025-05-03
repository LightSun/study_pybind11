

from setuptools import setup, Extension

extension_mod=Extension("PyOcrInfer", sources=['xxx.so'])

setup(name="PyOcrInfer",
   version="1.0.0",
   ext_modules=[extension_mod]
)

#python setup.py bdist.wheel
