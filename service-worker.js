self.addEventListener('install', (event) => {
    event.waitUntil(self.skipWaiting());
});

self.addEventListener('activate', (event) => {
    event.waitUntil(self.clients.claim());
});

self.addEventListener('push', (event) => {
    const data = event.data ? event.data.json() : { title: 'Alert:', body: 'Warning!!' };
    
    const options = {
        body: data.body,
        icon: 'alerticon.png', 
        badge: 'path/to/badge.png' 
    };

    event.waitUntil(
        self.registration.showNotification(data.title, options)
    );
});

self.addEventListener('notificationclick', (event) => {
    event.notification.close();
    
    event.waitUntil(
        clients.matchAll({ type: 'window' }).then((clientList) => {
            const hadWindowToFocus = clients.openWindow('http://127.0.0.1:5501/Mini%20Project/index.html'); // Replace with your website URL
            if (hadWindowToFocus) {
                hadWindowToFocus.then(client => client.focus());
            }
        })
    );
});
