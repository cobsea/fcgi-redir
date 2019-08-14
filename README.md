# FastCGI Redirector
FastCGI Redirector is the backend app for URL redirection. It uses MySQL for redirection rules and Memcached for caching.

Made as take-home assignment.

## Building

0. I've tested the application only on Debian 10 Buster, so I can guarantee it works on this distro.

1. Make sure you have installed the dependencies:

    * `gcc`
    * `libfcgi-dev`
    * `MySQL` and `libmysqlclient-dev`
    * `memcached` and `libmemcached-dev`
    * `Nginx`

1. Clone the [source code] with git:
    ```
    $ git clone https://github.com/cobsea/fcgi-redir.git
    $ cd fcgi-redir
    ```

    [source code]: https://github.com/cobsea/fcgi-redir

1. Build the source code to a binary with `make`:
    ```
    $ make
    ```
    The resulting binary is now in `fcgi-redir/build/` directory. Also this directory contains a `config/` folder.

    If you want to change the directory which a binary should be built to, you can simply change the `BUILD_DIR` variable in the Makefile.

## Setting up

1. Install the binary. 
    
    Makefile installs everything we built into `/var/www/fcgi/app`. You can change this path to yours by rewriting the `INSTALL_DIR` variable in the Makefile. Once you done it don't forget to change the `root` parameter in Nginx configuration file (`fcgi_redir_nginx.conf` if you are going to use Nginx configuration file from this repository).

    Then begin the installation with root privileges:
    ```
    $ sudo make install
    ```

1. Configure your Nginx.

    If you are going to use this repo's configuraion just copy the `fcgi_redir_nginx.conf` to the target directory:
    ```
    $ sudo cp fcgi_redir_nginx.conf /etc/nginx/conf.d/
    ```

    Please note that the FastCGI Redirector is installed in `/var/www/fcgi/app` by default, so you'll need to go to something like `http://1.2.3.4/app/fcgi_redir?your_query` to get access to the application. The Nginx is configured to have this `app/` subdirectory.

1. Set up MySQL.

    To install the given database, log in to your mysql user:
    ```
    $ mysql -h your_host -u your_user -p
    ```
    Then type:
    ```
    mysql> CREATE DATABASE your_database_name;
    mysql> USE your_database_name
    mysql> source fcgi_redir_data.sql
    ```
    If you want to work with your own database, make sure it has a table called "pairs" and this table contains at least 2 columns: `name` and `url`. The first one is used for search and it represents anything after `username` in the CGI query `...?username=Anil&ran...`. The second one is the URL string which begins with `http://` or `https://` and used for primary redirection.

1. Configure FastCGI Redirector.

    There is a configuration file in `/var/www/fcgi/app/config` by default. It uses such format:
    ```
    key1 value1
    key2 {
      subkey1 subvalue1
      subkey2 subvalue2
    }
    ```

    Every parameter that can be configured is already here, you can just change the values to yours. Note that parameter names are case-sensitive.

## Usage

FastCGI Redirector works with URLs which satisfy the following template:
```
http://1.2.3.4/app/fcgi_redir?username=Anil&ran=0.97584378943&pageURL=http://xyz.com
```

If a database on the host contains a given username and the URL paired with this username is correct, it redirects to this URL. Otherwise it redirects to a fallback URL which is `pageURL` parameter in CGI query. The `ran` parameter is nedded to avoid browser caching. All the redirections are cached to Memcached with a configurable timeout.

To shut down the application use `kill` program. After killing, FastCGI Redirector serves another one query and then shuts down.
