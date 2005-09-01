Dim FiltersObj
Dim FilterObj
Dim LoadOrder
Dim FilterName
Dim FilterPath
Dim FilterDesc
Dim WebObj
Dim existsFlag
Dim ScriptMaps
Dim newScriptLine
Dim line, lineArray, lineIndex
Dim WebSvcExts
Dim newWebSvcExtLine
Dim customData, msiProperties, InstallDir, ShibFileExtension

On Error Resume Next
Set WebObj = GetObject("IIS://LocalHost/W3SVC")
if (Err = 0) then

  'Get the INSTALLDIR and SHIB_FILE_EXTENSION values via CustomActionData
  customData = Session.Property("CustomActionData")
  msiProperties = split(customData,";@;")
  InstallDir = msiProperties(0)
  ShibFileExtension = msiProperties(1)

  'Remove all trailing backslashes to normalize
  do while (mid(InstallDir,Len(InstallDir),1) = "\")
    InstallDir = mid(InstallDir,1,Len(InstallDir)-1)
  loop
  ShibISAPIPath = InstallDir & "\libexec\isapi_shib.dll"
  'Make sure ShibFileExtension is in proper format
  'First, strip any preceding periods
  do while (mid(ShibFileExtension,1,1) = ".")
    ShibFileExtension = mid(ShibFileExtension,2,Len(ShibFileExtension)-1)
  loop
  'If there is nothing left (or was nothing to begin with), use the default
  if (ShibFileExtension = "") then
    ShibFileExtension = ".sso"
  else
    'Add preceding period
    ShibFileExtension = "." & ShibFileExtension
  end if

  'Specify other ISAPI Filter details
  FilterName = "Shibboleth"
  FilterPath = ShibISAPIPath
  FilterDesc = ""

  Set FiltersObj = GetObject("IIS://LocalHost/W3SVC/Filters")
  LoadOrder = FiltersObj.FilterLoadOrder
  'Check to see if 'Shibboleth' is already sequenced
  existsFlag = "not_exist"
  lineArray = split(LoadOrder, ",")
  for each line in lineArray
    if (line = FilterName) then
      existsFlag = "exists"
    end if
  next
  if (existsFlag = "not_exist") then
    If LoadOrder <> "" Then
      LoadOrder = LoadOrder & ","
    End If
    LoadOrder = LoadOrder & FilterName
    FiltersObj.FilterLoadOrder = LoadOrder
    FiltersObj.SetInfo
  else
    'msgbox "Shib Filter already sequenced"
  end if

  Set FilterObj = FiltersObj.Create("IIsFilter", FilterName)
  If (Err <> 0) then
    'Open existing filter for updating
    Err = 0
    Set FilterObj = GetObject("IIS://LocalHost/W3SVC/Filters/" & FilterName)
  End If
  FilterObj.FilterPath = FilterPath
  FilterObj.FilterDescription = FilterDesc
  FilterObj.SetInfo

  'Create file extension mapping to ISAPI filter
  ScriptMaps = WebObj.ScriptMaps
  'Check if exists
  newScriptLine = ShibFileExtension & "," & ShibISAPIPath & ",1"
  existsFlag = "not_exist"
  lineIndex = 0
  for each line in ScriptMaps
    lineArray = split(line,",")
    if (lineArray(0) = ShibFileExtension) then
      existsFlag = "exists"
      Exit For
    end if
    lineIndex = lineIndex + 1
  next
  if (existsFlag = "not_exist") then
    redim preserve ScriptMaps(UBound(ScriptMaps)+1)
    ScriptMaps(UBound(ScriptMaps)) = newScriptLine
    WebObj.ScriptMaps = ScriptMaps
    WebObj.SetInfo
  else
    'msgbox ".sso already exists: " & lineIndex
    'We already warned user in dialog that this value would be updated
    ScriptMaps(lineIndex) = newScriptLine
    WebObj.ScriptMaps = ScriptMaps
    WebObj.SetInfo
  end if


  'Web Services Extension
  Err = 0
  WebSvcExts = WebObj.WebSvcExtRestrictionList
  if (Err = 0) then
  newWebSvcExtLine = "1," & ShibISAPIPath & ",1,ShibGroup,Shibboleth Web Service Extension"

    existsFlag = "not_exist"
    lineIndex = 0
    for each line in WebSvcExts
      lineArray = split(line,",")
      if (lineArray(1) = ShibISAPIPath) then
        existsFlag = "exists"
        Exit For
      end if
      lineIndex = lineIndex + 1
    next

    if (existsFlag = "not_exist") then
      redim preserve WebSvcExts(UBound(WebSvcExts)+1)
      WebSvcExts(UBound(WebSVCExts)) = newWebSvcExtLine
      WebObj.WebSvcExtRestrictionList = WebSvcExts
      WebObj.SetInfo
    else
      'msgbox "Shibboleth Web Services Extension already exists: " & lineIndex
      'We already warned user in dialog that this value would be updated
      WebSvcExts(lineIndex) = newWebSvcExtLine
      WebObj.WebSvcExtRestrictionList = WebSvcExts
      WebObj.SetInfo
    end if

  end if

'final end if
end if