param(
    [string]$BuildDir = "Q:\ATM-2\build",
    [string]$MysqlExe = "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe",
    [string]$MySqlLibDir = "C:\Program Files\MySQL\MySQL Server 8.0\lib",
    [string]$DbHost = "127.0.0.1",
    [int]$DbPort = 3306,
    [string]$DbUser = "root",
    [string]$DbPassword = "@13921161526zhDQ",
    [string]$DbSchema = "atm_system",
    [string]$ServerHost = "127.0.0.1",
    [int]$ServerPort = 8888,
    [string]$WorkspaceRoot = "Q:\ATM-2"
)

$ErrorActionPreference = "Stop"

function Invoke-MySqlCommand {
    param([string]$Sql)

    if (!(Test-Path $MysqlExe)) {
        throw "mysql.exe not found: $MysqlExe"
    }

    & $MysqlExe --host=$DbHost --port=$DbPort --user=$DbUser --password=$DbPassword --default-character-set=utf8mb4 -e $Sql
    if ($LASTEXITCODE -ne 0) {
        throw "MySQL command failed: $Sql"
    }
}

function Invoke-MySqlScript {
    param([string]$ScriptPath)

    if (!(Test-Path $ScriptPath)) {
        throw "SQL script not found: $ScriptPath"
    }

    $sql = "source $($ScriptPath -replace '\\','/')"
    Invoke-MySqlCommand -Sql $sql
}

function Wait-ForTcpPort {
    param([string]$HostName, [int]$Port, [int]$TimeoutSeconds = 15)

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $client = [System.Net.Sockets.TcpClient]::new()
            $client.Connect($HostName, $Port)
            $client.Close()
            return
        } catch {
            Start-Sleep -Milliseconds 300
        }
    }

    throw "Timed out waiting for $HostName`:$Port"
}

function Invoke-AtmRequest {
    param([hashtable]$Payload)

    $client = [System.Net.Sockets.TcpClient]::new()
    $client.ReceiveTimeout = 5000
    $client.SendTimeout = 5000
    $client.Connect($ServerHost, $ServerPort)

    try {
        $stream = $client.GetStream()
        $json = ($Payload | ConvertTo-Json -Compress -Depth 5)
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
        $stream.Write($bytes, 0, $bytes.Length)
        $stream.Flush()

        $buffer = New-Object byte[] 8192
        $builder = [System.Text.StringBuilder]::new()
        do {
            $read = $stream.Read($buffer, 0, $buffer.Length)
            if ($read -le 0) {
                break
            }
            [void]$builder.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $read))
            Start-Sleep -Milliseconds 50
        } while ($stream.DataAvailable)

        $text = $builder.ToString()
        if ([string]::IsNullOrWhiteSpace($text)) {
            throw "Empty response for action '$($Payload.action)'"
        }

        return $text | ConvertFrom-Json
    } finally {
        $client.Close()
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw "ASSERT FAILED: $Message"
    }
}

function Assert-Equal {
    param($Expected, $Actual, [string]$Message)
    if ($Expected -ne $Actual) {
        throw "ASSERT FAILED: $Message. Expected '$Expected', actual '$Actual'"
    }
}

function To-Decimal {
    param($Value)
    return [decimal]::Parse($Value.ToString(), [System.Globalization.CultureInfo]::InvariantCulture)
}

$schemaPath = Join-Path $WorkspaceRoot "sql\schema.sql"
$seedPath = Join-Path $WorkspaceRoot "sql\seed.sql"
$serverExe = Join-Path $BuildDir "atm_server.exe"

if (!(Test-Path $serverExe)) {
    throw "Server executable not found: $serverExe"
}

$testId = Get-Date -Format "yyyyMMddHHmmss"
$testCardNo = "6888$testId"
$testAccountNo = "AT$($testId.Substring($testId.Length - 8))"
$testUserName = "AutoTestUser"
$testPassword = "Test1234"
$resetPassword = "Reset1234"

Write-Host "Preparing database..."
Invoke-MySqlCommand -Sql "DROP DATABASE IF EXISTS $DbSchema"
Invoke-MySqlScript -ScriptPath $schemaPath
Invoke-MySqlScript -ScriptPath $seedPath

$serverProcess = $null
try {
    Write-Host "Starting server..."
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $serverExe
    $startInfo.WorkingDirectory = $BuildDir
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.EnvironmentVariables["ATM_DB_HOST"] = $DbHost
    $startInfo.EnvironmentVariables["ATM_DB_PORT"] = $DbPort.ToString()
    $startInfo.EnvironmentVariables["ATM_DB_USER"] = $DbUser
    $startInfo.EnvironmentVariables["ATM_DB_PASSWORD"] = $DbPassword
    $startInfo.EnvironmentVariables["ATM_DB_SCHEMA"] = $DbSchema
    $startInfo.EnvironmentVariables["ATM_SERVER_HOST"] = $ServerHost
    $startInfo.EnvironmentVariables["ATM_SERVER_PORT"] = $ServerPort.ToString()
    if (Test-Path $MySqlLibDir) {
        $startInfo.EnvironmentVariables["PATH"] = "$MySqlLibDir;$($startInfo.EnvironmentVariables["PATH"])"
    }

    $serverProcess = [System.Diagnostics.Process]::Start($startInfo)
    Wait-ForTcpPort -HostName $ServerHost -Port $ServerPort

    Write-Host "Running integration flow..."

    $register = Invoke-AtmRequest @{
        action = "register"
        sessionId = ""
        data = @{
            cardNo = $testCardNo
            userName = $testUserName
            password = $testPassword
            accountNo = $testAccountNo
            initialBalance = "1000.00"
        }
    }
    Assert-True ($register.success -eq $true) "register should succeed"

    $userLogin = Invoke-AtmRequest @{
        action = "login"
        sessionId = ""
        data = @{ cardNo = $testCardNo; password = $testPassword }
    }
    Assert-True ($userLogin.success -eq $true) "user login should succeed"
    $userSession = $userLogin.data.sessionId
    Assert-True (-not [string]::IsNullOrWhiteSpace($userSession)) "user session should exist"

    $balance = Invoke-AtmRequest @{
        action = "query_balance"
        sessionId = $userSession
        data = @{}
    }
    Assert-Equal $testAccountNo $balance.data.accountNo "query balance should return account"
    Assert-Equal ([decimal]1000.00) (To-Decimal $balance.data.balance) "initial balance should be 1000.00"

    $deposit = Invoke-AtmRequest @{
        action = "deposit"
        sessionId = $userSession
        data = @{ amount = "200.00"; remark = "integration deposit" }
    }
    Assert-Equal ([decimal]1200.00) (To-Decimal $deposit.data.balance) "balance after deposit"

    $withdraw = Invoke-AtmRequest @{
        action = "withdraw"
        sessionId = $userSession
        data = @{ amount = "50.00"; remark = "integration withdraw" }
    }
    Assert-Equal ([decimal]1150.00) (To-Decimal $withdraw.data.balance) "balance after withdraw"

    $transfer = Invoke-AtmRequest @{
        action = "transfer"
        sessionId = $userSession
        data = @{ targetAccountNo = "ACC10002"; amount = "25.00"; remark = "integration transfer" }
    }
    Assert-Equal ([decimal]1125.00) (To-Decimal $transfer.data.balance) "balance after transfer"

    $transactions = Invoke-AtmRequest @{
        action = "query_transactions"
        sessionId = $userSession
        data = @{ page = "1"; pageSize = "5" }
    }
    Assert-True ($transactions.success -eq $true) "transaction query should succeed"
    Assert-Equal 1 ([int]$transactions.data.page) "transaction page"
    Assert-Equal 5 ([int]$transactions.data.pageSize) "transaction page size"
    Assert-True ([int]$transactions.data.total -ge 4) "transaction total should include seeded register and operations"

    $logout = Invoke-AtmRequest @{
        action = "logout"
        sessionId = $userSession
        data = @{}
    }
    Assert-True ($logout.success -eq $true) "logout should succeed"

    for ($attempt = 1; $attempt -le 3; $attempt++) {
        $badLogin = Invoke-AtmRequest @{
            action = "login"
            sessionId = ""
            data = @{ cardNo = $testCardNo; password = "wrongpass" }
        }
        Assert-True ($badLogin.success -eq $false) "bad login $attempt should fail"
    }

    $lockedLogin = Invoke-AtmRequest @{
        action = "login"
        sessionId = ""
        data = @{ cardNo = $testCardNo; password = $testPassword }
    }
    Assert-Equal 1003 ([int]$lockedLogin.code) "locked user should be rejected"

    $adminLogin = Invoke-AtmRequest @{
        action = "login"
        sessionId = ""
        data = @{ cardNo = "admin001"; password = "admin123" }
    }
    Assert-True ($adminLogin.success -eq $true) "admin login should succeed"
    $adminSession = $adminLogin.data.sessionId

    $listUsers = Invoke-AtmRequest @{
        action = "admin_list_users"
        sessionId = $adminSession
        data = @{ page = "1"; pageSize = "20" }
    }
    Assert-True ($listUsers.success -eq $true) "admin list users should succeed"
    Assert-True ([int]$listUsers.data.total -ge 3) "admin list users total should be at least seeded users plus test user"

    $unlock = Invoke-AtmRequest @{
        action = "admin_unlock_user"
        sessionId = $adminSession
        data = @{ cardNo = $testCardNo }
    }
    Assert-True ($unlock.success -eq $true) "admin unlock should succeed"

    $reset = Invoke-AtmRequest @{
        action = "admin_reset_password"
        sessionId = $adminSession
        data = @{ cardNo = $testCardNo; newPassword = $resetPassword }
    }
    Assert-True ($reset.success -eq $true) "admin reset password should succeed"

    $adminQueryTransactions = Invoke-AtmRequest @{
        action = "admin_query_transactions"
        sessionId = $adminSession
        data = @{ accountNo = $testAccountNo; page = "1"; pageSize = "5" }
    }
    Assert-True ($adminQueryTransactions.success -eq $true) "admin transaction query should succeed"
    Assert-True ([int]$adminQueryTransactions.data.total -ge 4) "admin transaction query total should be at least 4"

    $postResetLogin = Invoke-AtmRequest @{
        action = "login"
        sessionId = ""
        data = @{ cardNo = $testCardNo; password = $resetPassword }
    }
    Assert-True ($postResetLogin.success -eq $true) "user should log in with reset password"

    $invalidDeposit = Invoke-AtmRequest @{
        action = "deposit"
        sessionId = $postResetLogin.data.sessionId
        data = @{ amount = "-1"; remark = "invalid" }
    }
    Assert-Equal 1006 ([int]$invalidDeposit.code) "negative deposit should fail"

    Write-Host "Integration flow completed successfully."
} finally {
    if ($serverProcess -ne $null -and -not $serverProcess.HasExited) {
        $serverProcess.Kill()
        $serverProcess.WaitForExit()
    }

    if ($serverProcess -ne $null) {
        $stdout = $serverProcess.StandardOutput.ReadToEnd()
        $stderr = $serverProcess.StandardError.ReadToEnd()
        if (-not [string]::IsNullOrWhiteSpace($stdout)) {
            Write-Host "--- server stdout ---"
            Write-Host $stdout
        }
        if (-not [string]::IsNullOrWhiteSpace($stderr)) {
            Write-Host "--- server stderr ---"
            Write-Host $stderr
        }
    }
}
