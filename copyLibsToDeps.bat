IF EXIST "..\Debug\QuickSyncEncoded.lib" (
	COPY ..\Debug\QuickSyncEncoded.lib ..\..\deps\INTERNALS\lib\
)

IF EXIST "..\Release\QuickSyncEncode.lib" (
	COPY ..\Release\QuickSyncEncode.lib ..\..\deps\INTERNALS\lib\
)
