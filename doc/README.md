# Using twmailer_basic
Use following commands to compile and run the twmailer-server and twmailer-client.

## compile twmailer_basic
```
make
```

## remove binaries
```
make clean
```

## Start twmailer-server
```
./twmailer-server <port> <mail-spool-directoryname>

<mail-spool-directoryname> = message_store
```
**Input:**
```
./twmailer-server 7777 message_store
```

## Start twmailer-client
```
./twmailer-client <ip> <port>
```
**Input:**
```
./twmailer-client 127.0.0.1 7777
```

## Example Cases

### Case 1: Sending a Message

**Input:**

```
SEND
john_doe
alice_01
Coffee Break
Hi Alice,
Would you like to join me for a coffee break at 3 PM?
Thanks,
John
.
```

**Server Response:**

```
OK
```

OR the submitted data are invalid:

OR if opening of directory or file failed:

```
ERR
```

### Case 2: Listing Received Messages

**Input:**

```
LIST
vladan
```

**Server Response:**

```
5
halloWelt
Nachricht1
Nachricht2
Nachricht3
Nachricht4
```

OR if the username is invalid or doesn't exist:

```
0
```

### Case 3: Reading a Specific Message

**Input:**

```
READ
john_doe
1
```

**Server Response:**

```
OK
john_doe
peter_01
Coffee Break
Hi Peter,
Would you like to join me for a coffee break at 3 PM?
Thanks,
John
```

OR if the message number is invalid or doesn't exist:

```
ERR
```

### Case 4: Deleting a Specific Message

**Input:**

```
DEL
alice_01
1
```

**Server Response:**

```
OK
```

OR if the message number is invalid or doesn't exist:

```
ERR
```

### Case 5: Quitting

**Input:**

```
QUIT
```

(No server response since the server doesn't respond to the QUIT command.)

## Possible Errors
### Wrong Date & Time
If the date or time (check logMessage()-function) output (server-side) is wrong input following commands:
```
sudo apt-get install ntp
sudo service ntp stop
sudo ntpdate -s time.nist.gov
sudo service ntp start
```
### -lldap and -llber not installed
If LDAP-libraries are not installed, you will get an error.
Install with:
```
sudo apt-get update
sudo apt upgrade
sudo apt-get install libldap2-dev
```