set WNETBASE=C:\WINDDK\3790.1830
set WDF_ROOT=C:\WINDDK\WDF\KMDF10

ddkbuild.bat -WNET chk . /a -WDF

@rem ddkbuild.bat -WNET fre . /a -WDF
@rem ddkbuild.bat -WNETXP chk . /a -WDF
@rem ddkbuild.bat -WNETXP fre . /a -WDF
@rem ddkbuild.bat -WNET2K chk . /a -WDF
@rem ddkbuild.bat -WNET2K fre . /a -WDF
