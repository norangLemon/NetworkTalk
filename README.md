Network Talk
---

> 2016-2 컴퓨터네트워크 HW1<br>
> [과제 요구사항](http://incpaper.snu.ac.kr/images/6/68/CN2016_HW1.pdf)

## Problem statement

다음과 같은 것을 구현해야 한다.

### Client

1. login
    서버에 로그인을 요청한다. 
    이때 아이디만을 보내고, 로그인에 성공한 경우 "log on success"라는 메시지가 뜬다.
    실패한 경우 "log on denied"라는 메시지가 뜨며 재로그인을 요구한다.
    로그인에 성공하면 activate상태가 된다.
1. send msg
    보내는 사람을 입력하고, 메시지를 입력하면 전송된다.
1. read msg
    "S: sender M: msg text"의 포맷으로 받은 모든 메시지를 출력한다.
    이때 메시지는 지난 로그아웃 시점부터 모두 출력한다.

### Server

1. multiplexing
    서버는 하나지만 클라이언트는 여럿인 상황이므로, 동시에 여러 클라이언트의 작업을 처리할 수 있어야 한다.
1. activated user list
    전체 유저 중 성공적으로 로그인하여 아직 접속을 종료하지 않은 유저들을 따로 관리해야 한다.
    접속이 끊기면 즉시 이 목록에서 제외하고, 새로 로그인 하는 경우 다시 목록에 넣는다.
1. undelivered message list
    접속이 종료된 이후에 메시지가 도착하면 이것을 다시 리스트로 관리한다.
    유저가 로그인하는 즉시 전송해주어 확인할 수 있도록 한다.

## Details of Implementation

### Client

로그인 화면에서 아이디를 입력하여 가입된 아이디인 경우, 성공적으로 접속한다.
그렇지 않은 경우 실패 메시지를 띄우며 다시 아이디를 입력받는다.

로그인에 성공하면 `1. read message`, `2. send message`, `3. quit` 세 개의 메뉴를 출력한다.

1. read message
    서버가 보내준(주는) 메시지를 시간 순서대로 모두 띄워준다.
    먼저 서버가 보내준 메시지들은 저장해두었다가 메뉴 진입시 상단에 띄워준다.
    이 메뉴에 들어와 있을 때 서버가 보낸 메시지들은 바로바로 화면에 출력한다.
    받은 메시지는 역시 바로 리스트에 저장해둔다.
    q를 입력하면 메인 메뉴로 돌아간다. &rarr; select()써야햄!
1. send message
    순차적으로 받는 사람과 메시지를 입력한다.
    메시지 전송에 성공한 경우 "[successfully sent message]",
    실패한 경우 "[failed to send message]"라는 메시지를 출력한다.
    실패한 경우는 서버에서 ack을 보내지 않거나, nck이 온 경우로 가정한다.
    입력이 종료되면 성공이든 실패든 메인 메뉴로 돌아간다.
1. quit
    소켓을 닫고 프로그램을 종료한다.

### Server

#### 유저 관리
* 유저의 정보는 struct로 만들어서 리스트로 관리한다. 
전체 유저의 수는 100명이라고 가정한다.
struct의 멤버 변수는 user_ID, activate여부, 부재중 message list이다.
* logon
    * 로그인 요청이 들어오면 해당 유저가 가입된 유저인지 조회한다.
가입된 경우, activate 여부가 false일 때만 로그인을 허용한다.
가입되지 않았거나 activate된 경우 로그인을 불허한다.
    * 로그인 된 즉시 해당 유저의 정보 중 activate를 true로 바꾸어준다.
 



### Protocol

서버와 클라이언트 간 주고받는 메시지는 다음과 같이 구성된다.

`코드번호|요소1|요소2|...`

메시지는 한번에 하나의 `char` 배열로 전달된다.
요소는 메시지 타입에 따라서 서로 다르다(아래 항목에서 상술).
각 요소의 구분은 공백으로 이루어진다.
예컨대 유저 `cn10@cn.snucse.org`의 로그인 요청은 다음과 같을 것이다.

`0 cn10@cn.snucse.org`


#### Client to Server

* login request(0)
    * 서버에 로그인을 요청한다.
    * `0 ID`
* send message(1)
    * sender에게 message 전송 요청을 한다.
    * `1 receiver sender message`

#### Server to Client

* login accept/reject(0)
    * 서버에서 로그인을 accept하거나 reject한다.
    * rejected: `0 0`
    * accepted: `0 1`
* deliver message(1)
    * 유저에게 온 메시지를 전달해준다. 유저가 로그아웃 중일 경우 메시지를 보관해 두었다가, 로그인 하는 순간 전달해준다.
    * `1 receiver sender message`
* ack(2)
    * 유저가 전송한 메시지를 성공적으로 받았음을 알린다. 이때 존재하는 유저에게 전송한 경우(success)와 그렇지 않은 경우(fail)로 나뉜다.
    * fail: `2 0`
    * success: `2 1`

## Result


## Discussion
