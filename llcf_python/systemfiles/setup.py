"""

        $Date: 2006-08-28 20:05:22 +0200 (Mo, 28 Aug 2006) $
        $Rev: 850 $
        $Author: agrawal $
        $URL: vikas.agrawal@web.de $
        
        Copyright (c) 2006 S1nn GmbH & Co. KG.
        All Rights Reserved
        
"""

from distutils.core import setup, Extension

setup(name = '_llcf', version = '1.0', 
	ext_modules = [Extension('_llcf', ['llcfmodule.c'])]
)
